#include "net_date.h"


lv_obj_t * screen_main;  // 第一个界面


// SNTP 协议相关定义
#define NTP_TIMESTAMP_DELTA 2208988800ull // 1970到1900的秒数差
#define LI 0
#define VN 3
#define MODE 3

struct sntp_packet
{
    uint8_t leap_vn_mode;
    uint8_t stratum;
    uint8_t poll;
    uint8_t precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t ref_id;
    uint32_t ref_ts_sec;
    uint32_t ref_ts_frac;
    uint32_t orig_ts_sec;
    uint32_t orig_ts_frac;
    uint32_t recv_ts_sec;
    uint32_t recv_ts_frac;
    uint32_t trans_ts_sec;
    uint32_t trans_ts_frac;
};

// NTP 服务器地址
#define NTP_SERVER "ntp.aliyun.com"

/**
 * @brief 检查指定网卡是否获取到了 IP 地址
 */
int is_network_ready(const char *ifname)
{
    int sock;
    struct ifreq ifr;
    struct sockaddr_in *addr;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return 0;

    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

    if (ioctl(sock, SIOCGIFADDR, &ifr) < 0)
    {
        close(sock);
        return 0; // 没有配置 IP 或网卡不存在
    }

    addr = (struct sockaddr_in *)&ifr.ifr_addr;
    // 检查 IP 是否非 0.0.0.0
    int is_ready = (addr->sin_addr.s_addr != INADDR_ANY);

    close(sock);
    return is_ready;
}

void sync_system_time_native()
{
    int sockfd;
    struct sockaddr_in serv_addr;
    unsigned char packet[48] = {0};
    struct hostent *server;

    // --- 1. 创建 Socket ---
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0)
    {
        perror("❌ 创建 Socket 失败");
        return;
    }

    // --- 2. 配置服务器地址 (使用阿里云 NTP) ---
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(123); // NTP 标准端口 123

    // 解析域名
    server = gethostbyname("ntp.aliyun.com");
    if (server == NULL)
    {
        fprintf(stderr, "❌ 域名解析失败\n");
        close(sockfd);
        return;
    }
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    // --- 3. 构造 NTP 请求包 ---
    // 模式 3 (客户端), 版本 4
    packet[0] = (0 << 6) | (4 << 3) | 3;

    // --- 4. 发送请求 ---
    if (sendto(sockfd, packet, 48, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("❌ 发送 NTP 请求失败");
        close(sockfd);
        return;
    }

    // --- 5. 接收响应 ---
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    if (recvfrom(sockfd, packet, 48, 0, (struct sockaddr *)&client_addr, &client_len) < 0)
    {
        perror("❌ 接收 NTP 响应失败");
        close(sockfd);
        return;
    }

    // --- 6. 解析时间戳 (关键修改：使用 long long) ---
    // NTP 时间戳在 40-43 字节 (秒数)
    long long ntp_seconds = ((long long)packet[40] << 24) |
                            ((long long)packet[41] << 16) |
                            ((long long)packet[42] << 8) |
                            (long long)packet[43];

    // 转换为 Unix 时间戳 (UTC)
    long long utc_time = ntp_seconds - NTP_TIMESTAMP_DELTA;

    // 计算北京时间 (UTC + 8小时)
    long long beijing_time = utc_time + (8 * 3600);

    printf("🌐 [Native SNTP] 时间同步成功！(UTC时间戳: %lld)\n", utc_time);

    // --- 7. 设置系统时间 (关键修改：使用 clock_settime) ---
    struct timespec ts;
    ts.tv_sec = beijing_time; // 将“北京时间”写入内核
    ts.tv_nsec = 0;

    if (clock_settime(CLOCK_REALTIME, &ts) == 0)
    {
        printf("✅ 系统时间已设置为北京时间\n");
    }
    else
    {
        perror("❌ 设置系统时间失败 (需要Root权限)");
    }

    close(sockfd);
}

void lv_net_sync(void)
{
    const char *network_interface = "eth0";

    // 设置环境变量，虽然静态编译可能不生效，但保留是个好习惯
    setenv("TZ", "Asia/Shanghai", 1);
    tzset();

    printf("🕒 正在同步网络时间...\n");

    while (1)
    {
        if (is_network_ready(network_interface))
        {
            printf("🔗 网络就绪，开始同步...\n");

            // 调用同步函数 (内部已将系统时间设为“北京时间”)
            sync_system_time_native();

            // --- 验证代码 (关键修改：手动计算显示) ---
            time_t now_sys;
            struct tm tm_info;
            char buffer[30];

            // 1. 获取系统时间
            // 注意：因为上面 clock_settime 设置的是“北京时间的时间戳”，
            // 所以这里拿到的 now_sys 数值上已经是 UTC+8 了。
            time(&now_sys);

            // 2. 格式化输出
            // 我们使用 gmtime_r，因为它只负责把数字转成字符串，不进行任何时区加减。
            // 这样正好能原样输出我们刚才设置的“北京时间”。
            gmtime_r(&now_sys, &tm_info);

            strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm_info);

            printf("✅ 验证成功！当前显示时间是: %s\n", buffer);

            break;
        }
        sleep(1);
    }
}


uint8_t net_map[] =   { /* 0X00,0X01,0X28,0X00,0X28,0X00, */
0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
0X00,0X00,0X00,0X00,0X00,0X03,0XFF,0XE0,0X00,0X00,0X07,0XFF,0XF0,0X00,0X00,0X07,
0XFF,0XF0,0X00,0X00,0X07,0X00,0X70,0X00,0X00,0X07,0X00,0X70,0X00,0X00,0X07,0X00,
0X70,0X00,0X00,0X07,0X00,0X70,0X00,0X00,0X07,0X00,0X70,0X00,0X00,0X07,0X00,0X70,
0X00,0X00,0X07,0XFF,0XF0,0X00,0X00,0X07,0XFF,0XF0,0X00,0X00,0X03,0XFF,0XE0,0X00,
0X00,0X00,0X1C,0X00,0X00,0X00,0X00,0X1C,0X00,0X00,0X0F,0XFF,0XFF,0XFF,0XF8,0X1F,
0XFF,0XFF,0XFF,0XF8,0X1F,0XFF,0XFF,0XFF,0XF8,0X00,0X30,0X00,0X06,0X00,0X00,0X30,
0X00,0X06,0X00,0X00,0X38,0X00,0X0E,0X00,0X0F,0XFF,0XC1,0XFF,0XF8,0X0F,0XFF,0XC3,
0XFF,0XF8,0X0E,0X00,0XC3,0X80,0X38,0X0E,0X00,0XC3,0X80,0X38,0X0E,0X00,0XC3,0X80,
0X38,0X0E,0X00,0XC3,0X80,0X38,0X0E,0X00,0XC3,0X80,0X38,0X0E,0X00,0XC3,0X80,0X38,
0X0E,0X00,0XC3,0X80,0X38,0X0F,0XFF,0XC3,0XFF,0XF8,0X0F,0XFF,0XC1,0XFF,0XF0,0X00,
0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,};

const lv_img_dsc_t net = {
    .header.cf = LV_IMG_CF_ALPHA_1BIT,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = 40,
    .header.h = 40,
    .data_size = 200,
    .data = net_map,
};

void lv_net_img(void)
{
    lv_obj_t *img_icon = lv_img_create(screen_main);
    lv_img_set_src(img_icon, &net);
    lv_obj_align(img_icon, LV_ALIGN_TOP_RIGHT, 0, 0);

    // --- 关键部分 ---
    // 1. 设置想要染成的颜色 (例如：红色 #f2daf3)
    lv_obj_set_style_img_recolor(img_icon, lv_color_hex(0xf2daf3), 0);

    // 2. 设置染色的不透明度 (必须设为 COVER，否则颜色会很淡或者不显示)
    lv_obj_set_style_img_recolor_opa(img_icon, LV_OPA_COVER, 0);

    // 1. 确保背景是透明的（虽然 ALPHA_1BIT 默认就是透明的，但为了保险可以显式设置）
    // lv_obj_set_style_bg_opa(img_icon, LV_OPA_TRANSP, 0);

    // 2. 【不要】设置 img_recolor！
    // 如果你设置了 img_recolor 为白色，或者设置了 recolor_opa，
    // 可能会导致显示异常或者多余的计算。
    // 保持默认，它就是纯白色的。
}


/* 全局变量：标签对象指针 */
static lv_obj_t *label_time; // 显示 时:分
static lv_obj_t *label_sec;  // 显示 秒
static lv_obj_t *label_date; // 新增：日期标签

/**
 * @brief 时间更新回调函数
 */
static void time_update_timer_cb(lv_timer_t *timer)
{
    time_t now;
    struct tm timeinfo;
    char time_buf[64];
    char sec_buf[16];
    char date_buf[64]; // 新增：日期缓冲区

    // 1. 获取当前时间
    time(&now);
    localtime_r(&now, &timeinfo);

    // 2. 格式化 时:分
    snprintf(time_buf, sizeof(time_buf),
             "#87e2f0 %02d##dcdbe2 :##87e2f0 %02d#",
             timeinfo.tm_hour,
             timeinfo.tm_min);

    // 3. 格式化 秒
    snprintf(sec_buf, sizeof(sec_buf),
             "#dcdbe2 :%02d#",
             timeinfo.tm_sec);

    // --- 修改部分：格式化日期 (添加纯白颜色代码) ---
    // #ffffff 代表纯白色
    strftime(date_buf, sizeof(date_buf), "#FFFFFF %Y/%m/%d##FFFFFF  %A#", &timeinfo);

    // 4. 更新标签文本
    if (label_time != NULL)
    {
        lv_label_set_text(label_time, time_buf);
    }
    if (label_sec != NULL)
    {
        lv_label_set_text(label_sec, sec_buf);
    }
    // --- 新增：更新日期 ---
    if (label_date != NULL)
    {
        lv_label_set_text(label_date, date_buf);
    }
}

/**
 * @brief 初始化时间显示界面
 */
void lv_time_display_init(lv_obj_t * screen_main)
{
    /* --- 1. 创建“时:分”标签 --- */
    label_time = lv_label_create(screen_main);
    lv_label_set_recolor(label_time, true);
    lv_label_set_text(label_time, "#87e2f0 00:00#");
    lv_obj_set_style_text_font(label_time, &lv_font_number_200, 0);

    // LV_OPA_COVER (255) 是不透明，LV_OPA_TRANSP (0) 是完全透明
    // LV_OPA_90 表示 90% 不透明度 (稍微有点透)
    lv_obj_set_style_opa(label_time, LV_OPA_90, 0);
    // 设置显示位置
    lv_obj_align(label_time, LV_ALIGN_CENTER, -60, 180);

    /* --- 2. 创建“秒”标签 --- */
    label_sec = lv_label_create(screen_main);
    lv_label_set_recolor(label_sec, true);
    lv_label_set_text(label_sec, "#dcdbe2 :00#");
    lv_obj_set_style_text_font(label_sec, &lv_font_number_100, 0);
    // 这里设置稍微更透明一点，形成对比，或者保持一致均可
    lv_obj_set_style_opa(label_sec, LV_OPA_80, 0);
    // 位置设置
    lv_obj_align_to(label_sec, label_time, LV_ALIGN_OUT_RIGHT_BOTTOM, 0, 0);

    /* --- 3. 创建“日期”标签 --- */
    label_date = lv_label_create(screen_main);
    lv_label_set_recolor(label_date, true);
    // 日期不需要重着色，使用默认颜色即可
    lv_label_set_text(label_date, "#FFFFFF 2026/04/25 Sunday#"); // 设置初始文本
    lv_obj_set_style_text_font(label_date, &lv_font_AZ_40, 0);
    // 将日期标签放置在“时:分”标签的正上方
    lv_obj_align_to(label_date, label_time, LV_ALIGN_OUT_BOTTOM_MID, 60, 20);
    // 这里设置稍微更透明一点，形成对比，或者保持一致均可
    lv_obj_set_style_opa(label_date, LV_OPA_80, 0);

    /* --- 4. 创建定时器 --- */
    lv_timer_create(time_update_timer_cb, 1000, NULL);
}