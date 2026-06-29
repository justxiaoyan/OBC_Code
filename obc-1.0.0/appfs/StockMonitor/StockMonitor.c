#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <errno.h>
#include <cjson/cJSON.h>

/* ==================== 配置常量 ==================== */
#define UPDATE_INTERVAL_SEC 5      /* 更新间隔（秒）- 默认值 */
#define BUFFER_SIZE 4096           /* HTTP响应缓冲区大小 */
#define MAX_STOCKS 10              /* 最多监控的股票数量 */
#define CONFIG_FILE "StockConfig.json"  /* 配置文件名 */

/* ==================== 数据结构定义 ==================== */

/* 单个股票信息 */
typedef struct {
    char code[16];           /* 股票代码（如：sh600519） */
    char name[64];           /* 股票名称 */
    float current_price;     /* 当前价格 */
    float open_price;        /* 今日开盘价 */
    float yesterday_close;   /* 昨日收盘价 */
    float change_amount;     /* 涨跌额 */
    float change_percent;    /* 涨跌百分比 */
    char update_time[32];    /* 更新时间 */
} stock_info_t;

/* ==================== 函数声明 ==================== */
int load_config_from_json(const char *config_file, char stock_codes[][16], int *stock_count, int *update_interval);
int fetch_stock_data(const char *stock_code, char *response, size_t size);
int parse_stock_info(const char *response, stock_info_t *info);
void print_stock_info(const stock_info_t *info);
void print_usage(const char *prog_name);
const char* get_color_code(float change_percent);
void reset_color(void);

/* ==================== 主函数 ==================== */

int main(int argc, char *argv[])
{
    char stock_codes[MAX_STOCKS][16];
    int stock_count = 0;
    int update_interval = UPDATE_INTERVAL_SEC;
    char response[BUFFER_SIZE];
    stock_info_t stock_info;

    printf("========================================\n");
    printf("📈 A股股市行情监控工具\n");
    printf("========================================\n");

    /* 优先从JSON配置文件加载配置 */
    if (load_config_from_json(CONFIG_FILE, stock_codes, &stock_count, &update_interval) == 0) {
        printf("✅ 已从配置文件加载: %s\n", CONFIG_FILE);
    } else {
        printf("⚠️  未找到或无法解析配置文件: %s\n", CONFIG_FILE);

        /* 解析命令行参数作为备选 */
        if (argc < 2) {
            print_usage(argv[0]);
            /* 使用默认股票代码作为示例 */
            strcpy(stock_codes[0], "sh600519");  /* 贵州茅台 */
            strcpy(stock_codes[1], "sz000858");  /* 五粮液 */
            strcpy(stock_codes[2], "sh600036");  /* 招商银行 */
            stock_count = 3;
            printf("⚠️  使用默认示例股票\n\n");
        } else {
            for (int i = 1; i < argc && stock_count < MAX_STOCKS; i++) {
                strncpy(stock_codes[stock_count], argv[i], sizeof(stock_codes[stock_count]) - 1);
                stock_codes[stock_count][sizeof(stock_codes[stock_count]) - 1] = '\0';
                stock_count++;
            }
            printf("✅ 从命令行参数加载股票代码\n");
        }
    }

    printf("监控股票数量: %d\n", stock_count);
    printf("更新间隔: %d 秒\n", update_interval);
    printf("========================================\n\n");

    /* 主循环：定期获取并打印股票信息 */
    while (1) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        printf("\n🕐 更新时间: %04d-%02d-%02d %02d:%02d:%02d\n",
               t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
               t->tm_hour, t->tm_min, t->tm_sec);
        printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");

        for (int i = 0; i < stock_count; i++) {
            memset(&stock_info, 0, sizeof(stock_info_t));
            memset(response, 0, sizeof(response));

            /* 获取股票数据 */
            if (fetch_stock_data(stock_codes[i], response, sizeof(response)) == 0) {
                /* 解析股票信息 */
                if (parse_stock_info(response, &stock_info) == 0) {
                    print_stock_info(&stock_info);
                } else {
                    printf("❌ [%s] 解析股票数据失败\n", stock_codes[i]);
                }
            } else {
                printf("❌ [%s] 获取股票数据失败\n", stock_codes[i]);
            }
        }

        printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");

        /* 等待指定秒数 */
        sleep(update_interval);
    }

    return 0;
}

/* ==================== 函数实现 ==================== */

/**
 * @brief 从JSON配置文件加载配置
 * @param config_file 配置文件路径
 * @param stock_codes 股票代码数组（输出）
 * @param stock_count 股票数量（输出）
 * @param update_interval 更新间隔（输出）
 * @return 0=成功，-1=失败
 */
int load_config_from_json(const char *config_file, char stock_codes[][16], int *stock_count, int *update_interval)
{
    FILE *fp = NULL;
    char *file_content = NULL;
    long file_size;
    cJSON *root = NULL;
    cJSON *stocks_array = NULL;
    cJSON *stock_item = NULL;
    cJSON *code_item = NULL;
    cJSON *interval_item = NULL;
    int count = 0;
    int ret = -1;

    /* 打开配置文件 */
    fp = fopen(config_file, "r");
    if (fp == NULL) {
        return -1;
    }

    /* 获取文件大小 */
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (file_size <= 0 || file_size > 1024 * 1024) {  /* 限制1MB */
        fclose(fp);
        return -1;
    }

    /* 分配内存并读取文件内容 */
    file_content = (char *)malloc(file_size + 1);
    if (file_content == NULL) {
        fclose(fp);
        return -1;
    }

    if (fread(file_content, 1, file_size, fp) != (size_t)file_size) {
        free(file_content);
        fclose(fp);
        return -1;
    }
    file_content[file_size] = '\0';
    fclose(fp);

    /* 解析JSON */
    root = cJSON_Parse(file_content);
    free(file_content);

    if (root == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(stderr, "JSON解析错误: %s\n", error_ptr);
        }
        return -1;
    }

    /* 读取更新间隔 */
    interval_item = cJSON_GetObjectItem(root, "update_interval");
    if (cJSON_IsNumber(interval_item)) {
        *update_interval = interval_item->valueint;
        if (*update_interval < 1) {
            *update_interval = UPDATE_INTERVAL_SEC;
        }
    }

    /* 读取股票列表 */
    stocks_array = cJSON_GetObjectItem(root, "stocks");
    if (!cJSON_IsArray(stocks_array)) {
        cJSON_Delete(root);
        return -1;
    }

    cJSON_ArrayForEach(stock_item, stocks_array) {
        if (count >= MAX_STOCKS) {
            break;
        }

        code_item = cJSON_GetObjectItem(stock_item, "code");
        if (cJSON_IsString(code_item) && code_item->valuestring != NULL) {
            strncpy(stock_codes[count], code_item->valuestring, 15);
            stock_codes[count][15] = '\0';
            count++;
        }
    }

    *stock_count = count;
    ret = (count > 0) ? 0 : -1;

    cJSON_Delete(root);
    return ret;
}

/**
 * @brief 打印使用说明
 */
void print_usage(const char *prog_name)
{
    printf("用法: %s [股票代码1] [股票代码2] ...\n\n", prog_name);
    printf("说明:\n");
    printf("  股票代码格式:\n");
    printf("    - 上海股票: sh + 6位代码 (如: sh600519 贵州茅台)\n");
    printf("    - 深圳股票: sz + 6位代码 (如: sz000858 五粮液)\n\n");
    printf("示例:\n");
    printf("  %s sh600519              # 监控贵州茅台\n", prog_name);
    printf("  %s sh600519 sz000858     # 监控贵州茅台和五粮液\n", prog_name);
    printf("  %s sh600036 sh600887     # 监控招商银行和伊利股份\n\n", prog_name);
}

/**
 * @brief 获取颜色代码（红色=上涨，绿色=下跌，白色=平盘）
 */
const char* get_color_code(float change_percent)
{
    if (change_percent > 0.01) {
        return "\033[1;31m";  /* 红色（上涨） */
    } else if (change_percent < -0.01) {
        return "\033[1;32m";  /* 绿色（下跌） */
    } else {
        return "\033[1;37m";  /* 白色（平盘） */
    }
}

/**
 * @brief 重置颜色
 */
void reset_color(void)
{
    printf("\033[0m");
}

/**
 * @brief 从新浪财经API获取股票数据
 */
int fetch_stock_data(const char *stock_code, char *response, size_t size)
{
    int sock;
    struct sockaddr_in server_addr;
    struct hostent *host;
    char request[512];
    int bytes_received;
    char *body_start;

    if (stock_code == NULL || response == NULL || size == 0) {
        return -1;
    }

    /* 新浪财经API接口 */
    const char *hostname = "hq.sinajs.cn";
    const int port = 80;

    /* 解析主机名 */
    host = gethostbyname(hostname);
    if (host == NULL) {
        fprintf(stderr, "错误: 无法解析主机名 %s\n", hostname);
        return -1;
    }

    /* 创建socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    /* 设置超时 */
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    /* 设置服务器地址 */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr.s_addr, host->h_addr, host->h_length);

    /* 连接服务器 */
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return -1;
    }

    /* 构造HTTP GET请求（添加Referer避免被拒绝） */
    snprintf(request, sizeof(request),
             "GET /list=%s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36\r\n"
             "Referer: http://finance.sina.com.cn\r\n"
             "Connection: close\r\n"
             "\r\n",
             stock_code, hostname);

    /* 发送请求 */
    if (send(sock, request, strlen(request), 0) < 0) {
        perror("send");
        close(sock);
        return -1;
    }

    /* 接收响应 */
    bytes_received = recv(sock, response, size - 1, 0);
    if (bytes_received < 0) {
        perror("recv");
        close(sock);
        return -1;
    }
    response[bytes_received] = '\0';

    close(sock);

    /* 查找HTTP响应体（跳过HTTP头） */
    body_start = strstr(response, "\r\n\r\n");
    if (body_start) {
        body_start += 4;
        memmove(response, body_start, strlen(body_start) + 1);
    }

    return 0;
}

/**
 * @brief 解析新浪财经API返回的股票信息
 *
 * 数据格式示例：
 * var hq_str_sh600519="贵州茅台,1580.00,1575.00,1590.00,1595.00,1570.00,1589.00,1590.00,..."
 *
 * 字段说明（按逗号分隔）：
 * 0-股票名称, 1-今日开盘价, 2-昨日收盘价, 3-当前价格, 4-最高价, 5-最低价, ...
 */
int parse_stock_info(const char *response, stock_info_t *info)
{
    char *token;
    char buffer[BUFFER_SIZE];
    int field_index = 0;

    if (response == NULL || info == NULL) {
        return -1;
    }

    /* 查找引号内的数据 */
    const char *start = strchr(response, '"');
    if (start == NULL) {
        return -1;
    }
    start++; /* 跳过开始引号 */

    const char *end = strchr(start, '"');
    if (end == NULL) {
        return -1;
    }

    /* 提取数据部分 */
    size_t data_len = end - start;
    if (data_len >= sizeof(buffer)) {
        return -1;
    }

    strncpy(buffer, start, data_len);
    buffer[data_len] = '\0';

    /* 解析股票代码（从响应中提取） */
    const char *code_start = strstr(response, "hq_str_");
    if (code_start) {
        code_start += 7;
        const char *code_end = strchr(code_start, '=');
        if (code_end) {
            size_t code_len = code_end - code_start;
            if (code_len < sizeof(info->code)) {
                strncpy(info->code, code_start, code_len);
                info->code[code_len] = '\0';
            }
        }
    }

    /* 按逗号分隔字段 */
    token = strtok(buffer, ",");
    while (token != NULL && field_index <= 30) {
        switch (field_index) {
            case 0:  /* 股票名称 */
                strncpy(info->name, token, sizeof(info->name) - 1);
                info->name[sizeof(info->name) - 1] = '\0';
                break;
            case 1:  /* 今日开盘价 */
                info->open_price = atof(token);
                break;
            case 2:  /* 昨日收盘价 */
                info->yesterday_close = atof(token);
                break;
            case 3:  /* 当前价格 */
                info->current_price = atof(token);
                break;
            case 30: /* 更新时间（日期） */
                strncpy(info->update_time, token, sizeof(info->update_time) - 1);
                info->update_time[sizeof(info->update_time) - 1] = '\0';
                break;
            case 31: /* 更新时间（时间） */
                strncat(info->update_time, " ", sizeof(info->update_time) - strlen(info->update_time) - 1);
                strncat(info->update_time, token, sizeof(info->update_time) - strlen(info->update_time) - 1);
                break;
        }
        token = strtok(NULL, ",");
        field_index++;
    }

    /* 计算涨跌额和涨跌百分比 */
    if (info->yesterday_close > 0.01) {
        info->change_amount = info->current_price - info->yesterday_close;
        info->change_percent = (info->change_amount / info->yesterday_close) * 100.0f;
    } else {
        info->change_amount = 0.0f;
        info->change_percent = 0.0f;
    }

    return 0;
}

/**
 * @brief 打印股票信息（带颜色）
 */
void print_stock_info(const stock_info_t *info)
{
    if (info == NULL) {
        return;
    }

    /* 检查是否有效数据 */
    if (info->yesterday_close < 0.01) {
        printf("❌ [%s] %s - 无有效数据（可能代码错误）\n",
               info->code, info->name);
        return;
    }

    /* 判断是否是交易时间（当前价格为0表示非交易时间） */
    int is_trading = (info->current_price > 0.01);

    if (!is_trading) {
        /* 非交易时间，显示昨收价格 */
        printf("⏸️  [%s] %s\n", info->code, info->name);
        printf("  💤 非交易时间 - 昨收: %.2f 元",
               info->yesterday_close);
        if (strlen(info->update_time) > 0) {
            printf("  |  时间: %s", info->update_time);
        }
        printf("\n");
        return;
    }

    const char *color = get_color_code(info->change_percent);
    const char *trend_icon = info->change_percent > 0 ? "📈" : (info->change_percent < 0 ? "📉" : "➡️ ");

    printf("%s [%s] %s\n", trend_icon, info->code, info->name);
    printf("  当前价格: %s%.2f 元%s",
           color, info->current_price, "\033[0m");

    printf("  |  涨跌额: %s%+.2f 元%s",
           color, info->change_amount, "\033[0m");

    printf("  |  涨跌幅: %s%+.2f%%%s\n",
           color, info->change_percent, "\033[0m");

    printf("  开盘价: %.2f 元  |  昨收: %.2f 元",
           info->open_price, info->yesterday_close);

    if (strlen(info->update_time) > 0) {
        printf("  |  时间: %s", info->update_time);
    }
    printf("\n");
}
