
#include "stock_info.h"

/* ==================== 全局变量定义 ==================== */

/* 股票信息屏幕对象 */
lv_obj_t *screen_stock = NULL;

/* UI控件集合 */
stock_ui_widgets_t g_stock_widgets = {0};

/* 股票信息数据 */
stock_info_t g_stock_data[MAX_STOCKS] = {0};
int g_stock_count = 0;

/* 数据更新线程相关 */
static pthread_t g_update_thread = 0;
static int g_thread_running = 0;
static pthread_mutex_t g_data_mutex = PTHREAD_MUTEX_INITIALIZER;
static int g_update_interval = 5;  /* 更新间隔（秒） */

/* ==================== 内部辅助函数 ==================== */

static void *stock_update_thread(void *arg);

/**
 * @brief 去除字符串首尾空格
 */
static char *trim(char *str)
{
    char *end;
    while (*str == ' ' || *str == '\t' || *str == '\r' || *str == '\n') str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n')) end--;
    *(end + 1) = 0;
    return str;
}

/**
 * @brief 从INI配置文件加载股票配置
 */
int stock_load_config(void)
{
    FILE *fp = NULL;
    char line[256];
    char section[64] = "";
    int count = 0;
    int stock_index = -1;

    /* 打开配置文件 */
    fp = fopen(CONFIG_FILE, "r");
    if (fp == NULL) {
        printf("[Stock] 无法打开配置文件: %s\n", CONFIG_FILE);
        return -1;
    }

    /* 逐行解析INI文件 */
    while (fgets(line, sizeof(line), fp) != NULL) {
        char *p = trim(line);

        /* 跳过空行和注释 */
        if (*p == '\0' || *p == ';' || *p == '#') {
            continue;
        }

        /* 解析section */
        if (*p == '[') {
            char *end = strchr(p, ']');
            if (end != NULL) {
                *end = '\0';
                strncpy(section, p + 1, sizeof(section) - 1);
                section[sizeof(section) - 1] = '\0';

                /* 检查是否是股票section */
                if (strncmp(section, "Stock", 5) == 0 && count < MAX_STOCKS) {
                    stock_index = count;
                    g_stock_data[stock_index].valid = 0;
                    g_stock_data[stock_index].code[0] = '\0';
                    g_stock_data[stock_index].description[0] = '\0';
                    count++;
                } else if (strcmp(section, "Settings") == 0) {
                    stock_index = -1;
                }
            }
            continue;
        }

        /* 解析key=value */
        char *eq = strchr(p, '=');
        if (eq != NULL) {
            *eq = '\0';
            char *key = trim(p);
            char *value = trim(eq + 1);

            /* 处理Settings section */
            if (strcmp(section, "Settings") == 0) {
                if (strcmp(key, "update_interval") == 0) {
                    g_update_interval = atoi(value);
                    if (g_update_interval < 1) {
                        g_update_interval = 5;
                    }
                }
            }
            /* 处理Stock section */
            else if (stock_index >= 0 && stock_index < MAX_STOCKS) {
                if (strcmp(key, "code") == 0) {
                    strncpy(g_stock_data[stock_index].code, value, sizeof(g_stock_data[stock_index].code) - 1);
                    g_stock_data[stock_index].code[sizeof(g_stock_data[stock_index].code) - 1] = '\0';
                } else if (strcmp(key, "description") == 0) {
                    strncpy(g_stock_data[stock_index].description, value, sizeof(g_stock_data[stock_index].description) - 1);
                    g_stock_data[stock_index].description[sizeof(g_stock_data[stock_index].description) - 1] = '\0';
                }
            }
        }
    }

    fclose(fp);
    g_stock_count = count;

    printf("[Stock] 配置加载成功: %d 只股票\n", g_stock_count);
    return 0;
}

/**
 * @brief 获取单个股票数据
 */
int stock_fetch_data(const char *stock_code, stock_info_t *info)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char request[512];
    char response[BUFFER_SIZE];
    int bytes_received;
    char *data_start;
    char *data_end;

    if (stock_code == NULL || info == NULL) {
        return -1;
    }

    /* 创建socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1;
    }

    /* 设置超时 */
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    /* 解析主机名 */
    server = gethostbyname("hq.sinajs.cn");
    if (server == NULL) {
        close(sockfd);
        return -1;
    }

    /* 设置服务器地址 */
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
    serv_addr.sin_port = htons(80);

    /* 连接服务器 */
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        close(sockfd);
        return -1;
    }

    /* 构造HTTP请求 - 添加必要的请求头避免403 */
    snprintf(request, sizeof(request),
             "GET /list=%s HTTP/1.1\r\n"
             "Host: hq.sinajs.cn\r\n"
             "User-Agent: Mozilla/5.0\r\n"
             "Referer: http://finance.sina.com.cn\r\n"
             "Connection: close\r\n\r\n",
             stock_code);

    /* 发送请求 */
    if (send(sockfd, request, strlen(request), 0) < 0) {
        close(sockfd);
        return -1;
    }

    /* 接收响应 */
    memset(response, 0, sizeof(response));
    bytes_received = recv(sockfd, response, sizeof(response) - 1, 0);
    close(sockfd);

    if (bytes_received <= 0) {
        return -1;
    }

    /* 跳过HTTP头，找到响应体 */
    char *body = strstr(response, "\r\n\r\n");
    if (body != NULL) {
        body += 4;
    } else {
        return -1;
    }

    /* 解析响应数据 */
    data_start = strstr(body, "\"");
    if (data_start == NULL) {
        return -1;
    }
    data_start++;

    data_end = strstr(data_start, "\"");
    if (data_end == NULL) {
        return -1;
    }
    *data_end = '\0';

    /* 解析字段 */
    char *fields[33];
    int field_count = 0;
    char *token = strtok(data_start, ",");
    while (token != NULL && field_count < 33) {
        fields[field_count++] = token;
        token = strtok(NULL, ",");
    }

    if (field_count < 32) {
        return -1;
    }

    /* 填充数据结构 */
    strncpy(info->code, stock_code, sizeof(info->code) - 1);
    strncpy(info->name, fields[0], sizeof(info->name) - 1);

    /* 直接使用 atof 赋值，避免中间变量 */
    info->open_price = atof(fields[1]);
    info->yesterday_close = atof(fields[2]);
    info->current_price = atof(fields[3]);

    /* 简单的减法 */
    info->change_amount = info->current_price - info->yesterday_close;

    /* 使用整数计算百分比，避免浮点除法 */
    int yclose_int = (int)(info->yesterday_close * 100);
    int change_int = (int)(info->change_amount * 10000);
    if (yclose_int != 0) {
        info->change_percent = (float)change_int / (float)yclose_int;
    } else {
        info->change_percent = 0;
    }

    snprintf(info->update_time, sizeof(info->update_time), "%s %s", fields[30], fields[31]);
    info->valid = 1;

    return 0;
}

/**
 * @brief 股票数据更新线程
 */
static void *stock_update_thread(void *arg)
{
    (void)arg;

    while (g_thread_running) {
        for (int i = 0; i < g_stock_count; i++) {
            stock_info_t temp_info = {0};
            strcpy(temp_info.description, g_stock_data[i].description);

            if (stock_fetch_data(g_stock_data[i].code, &temp_info) == 0) {
                pthread_mutex_lock(&g_data_mutex);
                memcpy(&g_stock_data[i], &temp_info, sizeof(stock_info_t));
                strcpy(g_stock_data[i].description, temp_info.description);
                pthread_mutex_unlock(&g_data_mutex);
            } else {
                pthread_mutex_lock(&g_data_mutex);
                g_stock_data[i].valid = 0;
                pthread_mutex_unlock(&g_data_mutex);
            }
        }

        sleep(g_update_interval);
    }

    return NULL;
}

/**
 * @brief 启动股票数据更新线程
 */
int stock_start_update_thread(void)
{
    if (g_thread_running) {
        printf("[Stock] 更新线程已在运行\n");
        return 0;
    }

    g_thread_running = 1;

    if (pthread_create(&g_update_thread, NULL, stock_update_thread, NULL) != 0) {
        perror("[Stock] 线程创建失败");
        g_thread_running = 0;
        return -1;
    }

    printf("[Stock] 更新线程启动成功\n");
    return 0;
}

/**
 * @brief 停止股票数据更新线程
 */
void stock_stop_update_thread(void)
{
    if (!g_thread_running) {
        return;
    }

    printf("[Stock] 正在停止更新线程...\n");
    g_thread_running = 0;

    if (g_update_thread != 0) {
        pthread_join(g_update_thread, NULL);
        g_update_thread = 0;
    }

    printf("[Stock] 更新线程已停止\n");
}

/* ==================== UI创建函数 ==================== */

/**
 * @brief 初始化股票信息界面
 */
void screen_stock_screen_init(void)
{
    int panel_height = 60;
    int panel_spacing = 8;
    int start_y = -200;

    /* 创建标题标签 */
    g_stock_widgets.title_label = lv_label_create(screen_stock);
    lv_label_set_text(g_stock_widgets.title_label, "Stock Monitor");
    lv_obj_set_style_text_font(g_stock_widgets.title_label, &lv_font_montserrat_30, 0);
    lv_obj_set_style_text_color(g_stock_widgets.title_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(g_stock_widgets.title_label, LV_ALIGN_TOP_MID, 0, 10);

    /* 为每只股票创建显示面板 */
    for (int i = 0; i < g_stock_count && i < MAX_STOCKS; i++) {
        int y_pos = start_y + i * (panel_height + panel_spacing);

        /* 创建股票面板 */
        g_stock_widgets.stock_panels[i] = lv_obj_create(screen_stock);
        lv_obj_set_width(g_stock_widgets.stock_panels[i], 700);
        lv_obj_set_height(g_stock_widgets.stock_panels[i], panel_height);
        lv_obj_set_align(g_stock_widgets.stock_panels[i], LV_ALIGN_CENTER);
        lv_obj_set_y(g_stock_widgets.stock_panels[i], y_pos);
        lv_obj_clear_flag(g_stock_widgets.stock_panels[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(g_stock_widgets.stock_panels[i], lv_color_hex(0x404040), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(g_stock_widgets.stock_panels[i], 180, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(g_stock_widgets.stock_panels[i], 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_color(g_stock_widgets.stock_panels[i], lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);

        /* 股票名称标签 - 左侧 */
        g_stock_widgets.name_labels[i] = lv_label_create(g_stock_widgets.stock_panels[i]);
        lv_label_set_text(g_stock_widgets.name_labels[i], g_stock_data[i].description);
        lv_obj_set_style_text_font(g_stock_widgets.name_labels[i], &lv_font_montserrat_30, 0);
        lv_obj_set_style_text_color(g_stock_widgets.name_labels[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(g_stock_widgets.name_labels[i], LV_ALIGN_LEFT_MID, 15, 0);

        /* 价格标签 - 中间偏左 */
        g_stock_widgets.price_labels[i] = lv_label_create(g_stock_widgets.stock_panels[i]);
        lv_label_set_text(g_stock_widgets.price_labels[i], "---");
        lv_obj_set_style_text_font(g_stock_widgets.price_labels[i], &lv_font_montserrat_30, 0);
        lv_obj_set_style_text_color(g_stock_widgets.price_labels[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(g_stock_widgets.price_labels[i], LV_ALIGN_CENTER, -80, 0);

        /* 涨跌幅标签 - 右侧 */
        g_stock_widgets.change_labels[i] = lv_label_create(g_stock_widgets.stock_panels[i]);
        lv_label_set_text(g_stock_widgets.change_labels[i], "---");
        lv_obj_set_style_text_font(g_stock_widgets.change_labels[i], &lv_font_montserrat_30, 0);
        lv_obj_set_style_text_color(g_stock_widgets.change_labels[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(g_stock_widgets.change_labels[i], LV_ALIGN_RIGHT_MID, -15, 0);
    }

    printf("[Stock] UI初始化完成，共 %d 只股票\n", g_stock_count);
}

/**
 * @brief 更新股票信息显示
 */
void stock_update_display(void)
{
    pthread_mutex_lock(&g_data_mutex);

    for (int i = 0; i < g_stock_count && i < MAX_STOCKS; i++) {
        if (!g_stock_data[i].valid) {
            lv_label_set_text(g_stock_widgets.price_labels[i], "N/A");
            lv_label_set_text(g_stock_widgets.change_labels[i], "N/A");
            lv_obj_set_style_text_color(g_stock_widgets.price_labels[i],
                                        lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(g_stock_widgets.change_labels[i],
                                        lv_color_hex(0x808080), LV_PART_MAIN | LV_STATE_DEFAULT);
            continue;
        }

        /* 更新价格 */
        char price_buf[32];
        snprintf(price_buf, sizeof(price_buf), "%.2f", g_stock_data[i].current_price);
        lv_label_set_text(g_stock_widgets.price_labels[i], price_buf);

        /* 更新涨跌幅和涨跌额 */
        char change_buf[64];
        snprintf(change_buf, sizeof(change_buf), "%+.2f  %+.2f%%",
                 g_stock_data[i].change_amount, g_stock_data[i].change_percent);
        lv_label_set_text(g_stock_widgets.change_labels[i], change_buf);

        /* 根据涨跌设置颜色 - 使用整数比较避免编译器bug */
        int change_int = (int)(g_stock_data[i].change_percent * 100);
        lv_color_t color;

        if (change_int > 1) {
            color = lv_color_hex(0xf75858);  /* 红色 - 上涨 */
        } else if (change_int < -1) {
            color = lv_color_hex(0x00FF00);  /* 绿色 - 下跌 */
        } else {
            color = lv_color_hex(0xFFFFFF);  /* 白色 - 平盘 */
        lv_obj_set_style_text_color(g_stock_widgets.price_labels[i], color, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(g_stock_widgets.change_labels[i], color, LV_PART_MAIN | LV_STATE_DEFAULT);
    }

    pthread_mutex_unlock(&g_data_mutex);
}
}
