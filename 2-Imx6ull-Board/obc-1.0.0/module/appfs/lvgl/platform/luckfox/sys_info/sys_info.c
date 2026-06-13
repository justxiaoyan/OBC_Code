
#include "sys_info.h"

/* ==================== 全局变量定义 ==================== */

/* 系统信息屏幕对象 */
lv_obj_t *screen_sysinfo = NULL;

/* UI控件集合 */
sysinfo_ui_widgets_t g_sysinfo_widgets = {0};

/* 系统信息数据（初始化为 N/A 默认值） */
sys_info_single_t g_sysinfo_data = {
    {{"N/A"}, {"N/A"}},          /* base: 设备名和IP显示N/A */
    {0, 0, 0},                   /* cpu: 0%, 0°C, 0核心 */
    {0},                         /* mem: 0% */
    {0, 0, 0, 0},                /* gpu: 无GPU */
    {{"N/A"}, {"N/A"}}           /* net: 上传/下载速度显示N/A */
};

/* UDP接收线程相关 */
static pthread_t g_udp_thread = 0;
static int g_udp_running = 0;
static int g_data_received = 0;       /* 标志：是否已接收到有效数据 */
static uint32_t g_last_recv_time = 0; /* 最后一次接收数据的时间戳（毫秒） */
static pthread_mutex_t g_data_mutex = PTHREAD_MUTEX_INITIALIZER;

#define DATA_TIMEOUT_MS 5000  /* 数据超时时间：5秒 */

/* ==================== 内部辅助函数 ==================== */

/* 临时字符串缓冲区（避免栈上分配导致编译器问题） */
static char g_temp_text_buf[128];

/* UDP接收线程函数 */
static void *udp_receiver_thread(void *arg);

/* LVGL定时器回调函数 */
static void sysinfo_timer_callback(lv_timer_t *timer);

/**
 * @brief 创建设备基础信息UI（IP地址和设备名称）
 */
static void create_device_info_ui(void)
{
    g_sysinfo_widgets.device_info = lv_label_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.device_info, LV_SIZE_CONTENT);
    lv_obj_set_height(g_sysinfo_widgets.device_info, LV_SIZE_CONTENT);
    lv_obj_set_x(g_sysinfo_widgets.device_info, -194);
    lv_obj_set_y(g_sysinfo_widgets.device_info, -298);
    lv_obj_set_align(g_sysinfo_widgets.device_info, LV_ALIGN_CENTER);
    lv_label_set_text(g_sysinfo_widgets.device_info, "10.10.0.56         ayan-server");
    lv_obj_set_style_text_color(g_sysinfo_widgets.device_info, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(g_sysinfo_widgets.device_info, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(g_sysinfo_widgets.device_info, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
}

/**
 * @brief 创建CPU信息UI布局
 */
static void create_cpu_info_ui(void)
{
    /* CPU信息面板背景 */
    g_sysinfo_widgets.cpu.panel = lv_obj_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.cpu.panel, 246);
    lv_obj_set_height(g_sysinfo_widgets.cpu.panel, 137);
    lv_obj_set_x(g_sysinfo_widgets.cpu.panel, -192);
    lv_obj_set_y(g_sysinfo_widgets.cpu.panel, -204);
    lv_obj_set_align(g_sysinfo_widgets.cpu.panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(g_sysinfo_widgets.cpu.panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_opa(g_sysinfo_widgets.cpu.panel, 25, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* CPU使用率标签 */
    g_sysinfo_widgets.cpu.usage_label = lv_label_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.cpu.usage_label, LV_SIZE_CONTENT);
    lv_obj_set_height(g_sysinfo_widgets.cpu.usage_label, LV_SIZE_CONTENT);
    lv_obj_set_x(g_sysinfo_widgets.cpu.usage_label, -268);
    lv_obj_set_y(g_sysinfo_widgets.cpu.usage_label, -248);
    lv_obj_set_align(g_sysinfo_widgets.cpu.usage_label, LV_ALIGN_CENTER);
    lv_label_set_text(g_sysinfo_widgets.cpu.usage_label, "CPU Used");
    lv_obj_set_style_text_color(g_sysinfo_widgets.cpu.usage_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(g_sysinfo_widgets.cpu.usage_label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* CPU使用率进度条 */
    g_sysinfo_widgets.cpu.usage_bar = lv_bar_create(screen_sysinfo);
    lv_bar_set_value(g_sysinfo_widgets.cpu.usage_bar, (int)g_sysinfo_data.cpu.usage_percent, LV_ANIM_OFF);
    lv_bar_set_start_value(g_sysinfo_widgets.cpu.usage_bar, 0, LV_ANIM_OFF);
    lv_obj_set_width(g_sysinfo_widgets.cpu.usage_bar, 221);
    lv_obj_set_height(g_sysinfo_widgets.cpu.usage_bar, 15);
    lv_obj_set_x(g_sysinfo_widgets.cpu.usage_bar, -192);
    lv_obj_set_y(g_sysinfo_widgets.cpu.usage_bar, -224);
    lv_obj_set_align(g_sysinfo_widgets.cpu.usage_bar, LV_ALIGN_CENTER);

    /* CPU使用率数值 */
    g_sysinfo_widgets.cpu.usage_value = lv_label_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.cpu.usage_value, LV_SIZE_CONTENT);
    lv_obj_set_height(g_sysinfo_widgets.cpu.usage_value, LV_SIZE_CONTENT);
    lv_obj_set_x(g_sysinfo_widgets.cpu.usage_value, -101);
    lv_obj_set_y(g_sysinfo_widgets.cpu.usage_value, -247);
    lv_obj_set_align(g_sysinfo_widgets.cpu.usage_value, LV_ALIGN_CENTER);
    lv_label_set_text(g_sysinfo_widgets.cpu.usage_value, "6%");
    lv_obj_set_style_text_color(g_sysinfo_widgets.cpu.usage_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(g_sysinfo_widgets.cpu.usage_value, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* CPU温度标签 */
    g_sysinfo_widgets.cpu.temp_label = lv_label_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.cpu.temp_label, LV_SIZE_CONTENT);
    lv_obj_set_height(g_sysinfo_widgets.cpu.temp_label, LV_SIZE_CONTENT);
    lv_obj_set_x(g_sysinfo_widgets.cpu.temp_label, -263);
    lv_obj_set_y(g_sysinfo_widgets.cpu.temp_label, -185);
    lv_obj_set_align(g_sysinfo_widgets.cpu.temp_label, LV_ALIGN_CENTER);
    lv_label_set_text(g_sysinfo_widgets.cpu.temp_label, "CPU Temp");
    lv_obj_set_style_text_color(g_sysinfo_widgets.cpu.temp_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(g_sysinfo_widgets.cpu.temp_label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* CPU温度进度条 */
    g_sysinfo_widgets.cpu.temp_bar = lv_bar_create(screen_sysinfo);
    lv_bar_set_value(g_sysinfo_widgets.cpu.temp_bar, (int)g_sysinfo_data.cpu.temperature, LV_ANIM_OFF);
    lv_bar_set_start_value(g_sysinfo_widgets.cpu.temp_bar, 0, LV_ANIM_OFF);
    lv_obj_set_width(g_sysinfo_widgets.cpu.temp_bar, 221);
    lv_obj_set_height(g_sysinfo_widgets.cpu.temp_bar, 15);
    lv_obj_set_x(g_sysinfo_widgets.cpu.temp_bar, -193);
    lv_obj_set_y(g_sysinfo_widgets.cpu.temp_bar, -160);
    lv_obj_set_align(g_sysinfo_widgets.cpu.temp_bar, LV_ALIGN_CENTER);
    lv_obj_set_style_bg_color(g_sysinfo_widgets.cpu.temp_bar, lv_color_hex(0xDEF246), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(g_sysinfo_widgets.cpu.temp_bar, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    /* CPU温度数值 */
    g_sysinfo_widgets.cpu.temp_value = lv_label_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.cpu.temp_value, LV_SIZE_CONTENT);
    lv_obj_set_height(g_sysinfo_widgets.cpu.temp_value, LV_SIZE_CONTENT);
    lv_obj_set_x(g_sysinfo_widgets.cpu.temp_value, -108);
    lv_obj_set_y(g_sysinfo_widgets.cpu.temp_value, -182);
    lv_obj_set_align(g_sysinfo_widgets.cpu.temp_value, LV_ALIGN_CENTER);
    lv_label_set_text(g_sysinfo_widgets.cpu.temp_value, "53°C");
    lv_obj_set_style_text_color(g_sysinfo_widgets.cpu.temp_value, lv_color_hex(0xEFF75A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(g_sysinfo_widgets.cpu.temp_value, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
}

/**
 * @brief 创建内存信息UI布局
 */
static void create_mem_info_ui(void)
{
    /* 内存信息面板背景 */
    g_sysinfo_widgets.mem.panel = lv_obj_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.mem.panel, 246);
    lv_obj_set_height(g_sysinfo_widgets.mem.panel, 76);
    lv_obj_set_x(g_sysinfo_widgets.mem.panel, -190);
    lv_obj_set_y(g_sysinfo_widgets.mem.panel, -76);
    lv_obj_set_align(g_sysinfo_widgets.mem.panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(g_sysinfo_widgets.mem.panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_opa(g_sysinfo_widgets.mem.panel, 25, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 内存使用率标签 */
    g_sysinfo_widgets.mem.usage_label = lv_label_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.mem.usage_label, LV_SIZE_CONTENT);
    lv_obj_set_height(g_sysinfo_widgets.mem.usage_label, LV_SIZE_CONTENT);
    lv_obj_set_x(g_sysinfo_widgets.mem.usage_label, -265);
    lv_obj_set_y(g_sysinfo_widgets.mem.usage_label, -92);
    lv_obj_set_align(g_sysinfo_widgets.mem.usage_label, LV_ALIGN_CENTER);
    lv_label_set_text(g_sysinfo_widgets.mem.usage_label, "MEM Used");
    lv_obj_set_style_text_color(g_sysinfo_widgets.mem.usage_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(g_sysinfo_widgets.mem.usage_label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 内存使用率进度条 */
    g_sysinfo_widgets.mem.usage_bar = lv_bar_create(screen_sysinfo);
    lv_bar_set_value(g_sysinfo_widgets.mem.usage_bar, (int)g_sysinfo_data.mem.usage_percent, LV_ANIM_OFF);
    lv_bar_set_start_value(g_sysinfo_widgets.mem.usage_bar, 0, LV_ANIM_OFF);
    lv_obj_set_width(g_sysinfo_widgets.mem.usage_bar, 221);
    lv_obj_set_height(g_sysinfo_widgets.mem.usage_bar, 15);
    lv_obj_set_x(g_sysinfo_widgets.mem.usage_bar, -193);
    lv_obj_set_y(g_sysinfo_widgets.mem.usage_bar, -66);
    lv_obj_set_align(g_sysinfo_widgets.mem.usage_bar, LV_ALIGN_CENTER);

    /* 内存使用率数值 */
    g_sysinfo_widgets.mem.usage_value = lv_label_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.mem.usage_value, LV_SIZE_CONTENT);
    lv_obj_set_height(g_sysinfo_widgets.mem.usage_value, LV_SIZE_CONTENT);
    lv_obj_set_x(g_sysinfo_widgets.mem.usage_value, -110);
    lv_obj_set_y(g_sysinfo_widgets.mem.usage_value, -91);
    lv_obj_set_align(g_sysinfo_widgets.mem.usage_value, LV_ALIGN_CENTER);
    lv_label_set_text(g_sysinfo_widgets.mem.usage_value, "31%");
    lv_obj_set_style_text_color(g_sysinfo_widgets.mem.usage_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(g_sysinfo_widgets.mem.usage_value, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
}

/**
 * @brief 创建GPU信息UI布局
 */
static void create_gpu_info_ui(void)
{
    /* GPU信息面板背景 */
    g_sysinfo_widgets.gpu.panel = lv_obj_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.gpu.panel, 246);
    lv_obj_set_height(g_sysinfo_widgets.gpu.panel, 184);
    lv_obj_set_x(g_sysinfo_widgets.gpu.panel, -189);
    lv_obj_set_y(g_sysinfo_widgets.gpu.panel, 82);
    lv_obj_set_align(g_sysinfo_widgets.gpu.panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(g_sysinfo_widgets.gpu.panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_opa(g_sysinfo_widgets.gpu.panel, 25, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* GPU使用率标签 */
    g_sysinfo_widgets.gpu.usage_label = lv_label_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.gpu.usage_label, LV_SIZE_CONTENT);
    lv_obj_set_height(g_sysinfo_widgets.gpu.usage_label, LV_SIZE_CONTENT);
    lv_obj_set_x(g_sysinfo_widgets.gpu.usage_label, -258);
    lv_obj_set_y(g_sysinfo_widgets.gpu.usage_label, 17);
    lv_obj_set_align(g_sysinfo_widgets.gpu.usage_label, LV_ALIGN_CENTER);
    lv_label_set_text(g_sysinfo_widgets.gpu.usage_label, "GPU Used");
    lv_obj_set_style_text_color(g_sysinfo_widgets.gpu.usage_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(g_sysinfo_widgets.gpu.usage_label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* GPU使用率进度条 */
    g_sysinfo_widgets.gpu.usage_bar = lv_bar_create(screen_sysinfo);
    lv_bar_set_value(g_sysinfo_widgets.gpu.usage_bar, (int)g_sysinfo_data.gpu.usage_percent, LV_ANIM_OFF);
    lv_bar_set_start_value(g_sysinfo_widgets.gpu.usage_bar, 0, LV_ANIM_OFF);
    lv_obj_set_width(g_sysinfo_widgets.gpu.usage_bar, 221);
    lv_obj_set_height(g_sysinfo_widgets.gpu.usage_bar, 15);
    lv_obj_set_x(g_sysinfo_widgets.gpu.usage_bar, -188);
    lv_obj_set_y(g_sysinfo_widgets.gpu.usage_bar, 39);
    lv_obj_set_align(g_sysinfo_widgets.gpu.usage_bar, LV_ALIGN_CENTER);

    /* GPU使用率数值 */
    g_sysinfo_widgets.gpu.usage_value = lv_label_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.gpu.usage_value, LV_SIZE_CONTENT);
    lv_obj_set_height(g_sysinfo_widgets.gpu.usage_value, LV_SIZE_CONTENT);
    lv_obj_set_x(g_sysinfo_widgets.gpu.usage_value, -103);
    lv_obj_set_y(g_sysinfo_widgets.gpu.usage_value, 13);
    lv_obj_set_align(g_sysinfo_widgets.gpu.usage_value, LV_ALIGN_CENTER);
    lv_label_set_text(g_sysinfo_widgets.gpu.usage_value, "0%");
    lv_obj_set_style_text_color(g_sysinfo_widgets.gpu.usage_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(g_sysinfo_widgets.gpu.usage_value, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* GPU温度标签 */
    g_sysinfo_widgets.gpu.temp_label = lv_label_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.gpu.temp_label, LV_SIZE_CONTENT);
    lv_obj_set_height(g_sysinfo_widgets.gpu.temp_label, LV_SIZE_CONTENT);
    lv_obj_set_x(g_sysinfo_widgets.gpu.temp_label, -259);
    lv_obj_set_y(g_sysinfo_widgets.gpu.temp_label, 73);
    lv_obj_set_align(g_sysinfo_widgets.gpu.temp_label, LV_ALIGN_CENTER);
    lv_label_set_text(g_sysinfo_widgets.gpu.temp_label, "GPU Temp");
    lv_obj_set_style_text_color(g_sysinfo_widgets.gpu.temp_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(g_sysinfo_widgets.gpu.temp_label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* GPU温度进度条 */
    g_sysinfo_widgets.gpu.temp_bar = lv_bar_create(screen_sysinfo);
    lv_bar_set_value(g_sysinfo_widgets.gpu.temp_bar, (int)g_sysinfo_data.gpu.temperature, LV_ANIM_OFF);
    lv_bar_set_start_value(g_sysinfo_widgets.gpu.temp_bar, 0, LV_ANIM_OFF);
    lv_obj_set_width(g_sysinfo_widgets.gpu.temp_bar, 221);
    lv_obj_set_height(g_sysinfo_widgets.gpu.temp_bar, 15);
    lv_obj_set_x(g_sysinfo_widgets.gpu.temp_bar, -189);
    lv_obj_set_y(g_sysinfo_widgets.gpu.temp_bar, 96);
    lv_obj_set_align(g_sysinfo_widgets.gpu.temp_bar, LV_ALIGN_CENTER);

    /* GPU温度数值 */
    g_sysinfo_widgets.gpu.temp_value = lv_label_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.gpu.temp_value, LV_SIZE_CONTENT);
    lv_obj_set_height(g_sysinfo_widgets.gpu.temp_value, LV_SIZE_CONTENT);
    lv_obj_set_x(g_sysinfo_widgets.gpu.temp_value, -105);
    lv_obj_set_y(g_sysinfo_widgets.gpu.temp_value, 67);
    lv_obj_set_align(g_sysinfo_widgets.gpu.temp_value, LV_ALIGN_CENTER);
    lv_label_set_text(g_sysinfo_widgets.gpu.temp_value, "43°C");
    lv_obj_set_style_text_color(g_sysinfo_widgets.gpu.temp_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(g_sysinfo_widgets.gpu.temp_value, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* GPU显存标签 */
    g_sysinfo_widgets.gpu.mem_label = lv_label_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.gpu.mem_label, LV_SIZE_CONTENT);
    lv_obj_set_height(g_sysinfo_widgets.gpu.mem_label, LV_SIZE_CONTENT);
    lv_obj_set_x(g_sysinfo_widgets.gpu.mem_label, -262);
    lv_obj_set_y(g_sysinfo_widgets.gpu.mem_label, 126);
    lv_obj_set_align(g_sysinfo_widgets.gpu.mem_label, LV_ALIGN_CENTER);
    lv_label_set_text(g_sysinfo_widgets.gpu.mem_label, "GPU MEM");
    lv_obj_set_style_text_color(g_sysinfo_widgets.gpu.mem_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(g_sysinfo_widgets.gpu.mem_label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* GPU显存进度条 */
    g_sysinfo_widgets.gpu.mem_bar = lv_bar_create(screen_sysinfo);
    lv_bar_set_value(g_sysinfo_widgets.gpu.mem_bar, (int)g_sysinfo_data.gpu.mem_usage_percent, LV_ANIM_OFF);
    lv_bar_set_start_value(g_sysinfo_widgets.gpu.mem_bar, 0, LV_ANIM_OFF);
    lv_obj_set_width(g_sysinfo_widgets.gpu.mem_bar, 221);
    lv_obj_set_height(g_sysinfo_widgets.gpu.mem_bar, 15);
    lv_obj_set_x(g_sysinfo_widgets.gpu.mem_bar, -187);
    lv_obj_set_y(g_sysinfo_widgets.gpu.mem_bar, 147);
    lv_obj_set_align(g_sysinfo_widgets.gpu.mem_bar, LV_ALIGN_CENTER);

    /* GPU显存数值 */
    g_sysinfo_widgets.gpu.mem_value = lv_label_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.gpu.mem_value, LV_SIZE_CONTENT);
    lv_obj_set_height(g_sysinfo_widgets.gpu.mem_value, LV_SIZE_CONTENT);
    lv_obj_set_x(g_sysinfo_widgets.gpu.mem_value, -98);
    lv_obj_set_y(g_sysinfo_widgets.gpu.mem_value, 124);
    lv_obj_set_align(g_sysinfo_widgets.gpu.mem_value, LV_ALIGN_CENTER);
    lv_label_set_text(g_sysinfo_widgets.gpu.mem_value, "1%");
    lv_obj_set_style_text_color(g_sysinfo_widgets.gpu.mem_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(g_sysinfo_widgets.gpu.mem_value, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
}

/**
 * @brief 创建网络信息UI布局
 */
static void create_net_info_ui(void)
{
    /* 网络信息面板背景 */
    g_sysinfo_widgets.net.panel = lv_obj_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.net.panel, 246);
    lv_obj_set_height(g_sysinfo_widgets.net.panel, 103);
    lv_obj_set_x(g_sysinfo_widgets.net.panel, -186);
    lv_obj_set_y(g_sysinfo_widgets.net.panel, 251);
    lv_obj_set_align(g_sysinfo_widgets.net.panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(g_sysinfo_widgets.net.panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_opa(g_sysinfo_widgets.net.panel, 25, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 上行带宽标签 */
    g_sysinfo_widgets.net.upload_label = lv_label_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.net.upload_label, LV_SIZE_CONTENT);
    lv_obj_set_height(g_sysinfo_widgets.net.upload_label, LV_SIZE_CONTENT);
    lv_obj_set_x(g_sysinfo_widgets.net.upload_label, -253);
    lv_obj_set_y(g_sysinfo_widgets.net.upload_label, 229);
    lv_obj_set_align(g_sysinfo_widgets.net.upload_label, LV_ALIGN_CENTER);
    lv_label_set_text(g_sysinfo_widgets.net.upload_label, "Net Up");
    lv_obj_set_style_text_color(g_sysinfo_widgets.net.upload_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(g_sysinfo_widgets.net.upload_label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 上行带宽数值 */
    g_sysinfo_widgets.net.upload_value = lv_label_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.net.upload_value, LV_SIZE_CONTENT);
    lv_obj_set_height(g_sysinfo_widgets.net.upload_value, LV_SIZE_CONTENT);
    lv_obj_set_x(g_sysinfo_widgets.net.upload_value, -252);
    lv_obj_set_y(g_sysinfo_widgets.net.upload_value, 263);
    lv_obj_set_align(g_sysinfo_widgets.net.upload_value, LV_ALIGN_CENTER);
    lv_label_set_text(g_sysinfo_widgets.net.upload_value, g_sysinfo_data.net.upload_speed);
    lv_obj_set_style_text_color(g_sysinfo_widgets.net.upload_value, lv_color_hex(0xF0F35F), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(g_sysinfo_widgets.net.upload_value, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(g_sysinfo_widgets.net.upload_value, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 下行带宽标签 */
    g_sysinfo_widgets.net.download_label = lv_label_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.net.download_label, LV_SIZE_CONTENT);
    lv_obj_set_height(g_sysinfo_widgets.net.download_label, LV_SIZE_CONTENT);
    lv_obj_set_x(g_sysinfo_widgets.net.download_label, -127);
    lv_obj_set_y(g_sysinfo_widgets.net.download_label, 229);
    lv_obj_set_align(g_sysinfo_widgets.net.download_label, LV_ALIGN_CENTER);
    lv_label_set_text(g_sysinfo_widgets.net.download_label, "Net Down");
    lv_obj_set_style_text_color(g_sysinfo_widgets.net.download_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(g_sysinfo_widgets.net.download_label, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(g_sysinfo_widgets.net.download_label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    /* 下行带宽数值 */
    g_sysinfo_widgets.net.download_value = lv_label_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.net.download_value, LV_SIZE_CONTENT);
    lv_obj_set_height(g_sysinfo_widgets.net.download_value, LV_SIZE_CONTENT);
    lv_obj_set_x(g_sysinfo_widgets.net.download_value, -128);
    lv_obj_set_y(g_sysinfo_widgets.net.download_value, 263);
    lv_obj_set_align(g_sysinfo_widgets.net.download_value, LV_ALIGN_CENTER);
    lv_label_set_text(g_sysinfo_widgets.net.download_value, g_sysinfo_data.net.download_speed);
    lv_obj_set_style_text_color(g_sysinfo_widgets.net.download_value, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(g_sysinfo_widgets.net.download_value, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(g_sysinfo_widgets.net.download_value, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
}

/**
 * @brief 创建主背景面板
 */
static void create_main_panel(void)
{
    /* 主背景面板 */
    g_sysinfo_widgets.main_panel = lv_obj_create(screen_sysinfo);
    lv_obj_set_width(g_sysinfo_widgets.main_panel, 286);
    lv_obj_set_height(g_sysinfo_widgets.main_panel, 648);
    lv_obj_set_x(g_sysinfo_widgets.main_panel, -190);
    lv_obj_set_y(g_sysinfo_widgets.main_panel, -2);
    lv_obj_set_align(g_sysinfo_widgets.main_panel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(g_sysinfo_widgets.main_panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_opa(g_sysinfo_widgets.main_panel, 25, LV_PART_MAIN | LV_STATE_DEFAULT);
}

/* ==================== 公共接口函数 ==================== */

/**
 * @brief 初始化系统信息界面
 * @note 按照从底层到顶层的顺序创建UI元素，确保正确的层次关系
 */
void screen_sysinfo_screen_init(void)
{
    /* 初始化默认数据（使用整数避免编译器问题） */
    g_sysinfo_data.cpu.usage_percent = 6;
    g_sysinfo_data.cpu.temperature = 53;
    g_sysinfo_data.mem.usage_percent = 31;
    g_sysinfo_data.gpu.temperature = 43;
    g_sysinfo_data.gpu.mem_usage_percent = 1;

    /* 创建主背景面板（最底层） */
    create_main_panel();

    /* 创建各个信息区域的面板和内容 */
    create_cpu_info_ui();
    create_mem_info_ui();
    create_gpu_info_ui();
    create_net_info_ui();

    /* 创建设备信息（最顶层） */
    create_device_info_ui();
}

/**
 * @brief 更新设备基础信息显示（包括网络带宽）
 */
static void update_device_info(const sys_info_single_t *data)
{
    char temp_buf[128];
    lv_color_t color;

    /* 更新设备名和IP */
    if (g_sysinfo_widgets.device_info != NULL) {
        snprintf(temp_buf, sizeof(temp_buf), "%s         %s",
                 data->base.ip_address, data->base.device_name);
        lv_label_set_text(g_sysinfo_widgets.device_info, temp_buf);
    }

    /* 更新上行带宽（根据单位设置颜色） */
    if (g_sysinfo_widgets.net.upload_value != NULL) {
        if (!g_data_received) {
            /* 无数据时显示 N/A，白色 */
            lv_label_set_text(g_sysinfo_widgets.net.upload_value, "N/A");
            lv_obj_set_style_text_color(g_sysinfo_widgets.net.upload_value,
                                        lv_color_hex(0xFFFFFF),
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            /* 检查单位：包含 "MB" 则黄色，否则白色 */
            if (strstr(data->net.upload_speed, "MB") != NULL) {
                color = lv_color_hex(0xEFF75A);  /* 黄色 */
            } else {
                color = lv_color_hex(0xFFFFFF);  /* 白色 */
            }
            lv_label_set_text(g_sysinfo_widgets.net.upload_value, data->net.upload_speed);
            lv_obj_set_style_text_color(g_sysinfo_widgets.net.upload_value,
                                        color,
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }

    /* 更新下行带宽（根据单位设置颜色） */
    if (g_sysinfo_widgets.net.download_value != NULL) {
        if (!g_data_received) {
            /* 无数据时显示 N/A，白色 */
            lv_label_set_text(g_sysinfo_widgets.net.download_value, "N/A");
            lv_obj_set_style_text_color(g_sysinfo_widgets.net.download_value,
                                        lv_color_hex(0xFFFFFF),
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
        } else {
            /* 检查单位：包含 "MB" 则黄色，否则白色 */
            if (strstr(data->net.download_speed, "MB") != NULL) {
                color = lv_color_hex(0xEFF75A);  /* 黄色 */
            } else {
                color = lv_color_hex(0xFFFFFF);  /* 白色 */
            }
            lv_label_set_text(g_sysinfo_widgets.net.download_value, data->net.download_speed);
            lv_obj_set_style_text_color(g_sysinfo_widgets.net.download_value,
                                        color,
                                        LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
}

/**
 * @brief 更新CPU信息显示
 */
static void update_cpu_info(const sys_info_single_t *data)
{
    char temp_buf[64];
    static int first_call = 1;

    if (first_call) {
        printf("[Update] 第一次更新 CPU 信息: %.1f%%, %.1f°C\n",
               data->cpu.usage_percent, data->cpu.temperature);
        first_call = 0;
    }

    /* 检查是否接收到有效数据 */
    if (!g_data_received) {
        /* 未接收到数据，显示 N/A */
        if (g_sysinfo_widgets.cpu.usage_bar != NULL) {
            lv_bar_set_value(g_sysinfo_widgets.cpu.usage_bar, 0, LV_ANIM_OFF);
        }
        if (g_sysinfo_widgets.cpu.usage_value != NULL) {
            lv_label_set_text(g_sysinfo_widgets.cpu.usage_value, "N/A");
        }
        if (g_sysinfo_widgets.cpu.temp_bar != NULL) {
            lv_bar_set_value(g_sysinfo_widgets.cpu.temp_bar, 0, LV_ANIM_OFF);
        }
        if (g_sysinfo_widgets.cpu.temp_value != NULL) {
            lv_label_set_text(g_sysinfo_widgets.cpu.temp_value, "N/A");
        }
        return;
    }

    /* 更新 CPU 使用率 */
    if (g_sysinfo_widgets.cpu.usage_bar != NULL) {
        lv_bar_set_value(g_sysinfo_widgets.cpu.usage_bar, (int)data->cpu.usage_percent, LV_ANIM_ON);
        sysinfo_set_color_by_value(g_sysinfo_widgets.cpu.usage_bar,
                                    g_sysinfo_widgets.cpu.usage_value,
                                    data->cpu.usage_percent);
    }
    if (g_sysinfo_widgets.cpu.usage_value != NULL) {
        snprintf(temp_buf, sizeof(temp_buf), "%.1f%%", data->cpu.usage_percent);
        lv_label_set_text(g_sysinfo_widgets.cpu.usage_value, temp_buf);
    }

    /* 更新 CPU 温度 */
    if (g_sysinfo_widgets.cpu.temp_bar != NULL) {
        lv_bar_set_value(g_sysinfo_widgets.cpu.temp_bar, (int)data->cpu.temperature, LV_ANIM_ON);
        sysinfo_set_color_by_value(g_sysinfo_widgets.cpu.temp_bar,
                                    g_sysinfo_widgets.cpu.temp_value,
                                    data->cpu.temperature);
    }
    if (g_sysinfo_widgets.cpu.temp_value != NULL) {
        snprintf(temp_buf, sizeof(temp_buf), "%.1f°C", data->cpu.temperature);
        lv_label_set_text(g_sysinfo_widgets.cpu.temp_value, temp_buf);
    }
}

/**
 * @brief 更新内存信息显示
 */
static void update_mem_info(const sys_info_single_t *data)
{
    char temp_buf[64];

    /* 检查是否接收到有效数据 */
    if (!g_data_received) {
        if (g_sysinfo_widgets.mem.usage_bar != NULL) {
            lv_bar_set_value(g_sysinfo_widgets.mem.usage_bar, 0, LV_ANIM_OFF);
        }
        if (g_sysinfo_widgets.mem.usage_value != NULL) {
            lv_label_set_text(g_sysinfo_widgets.mem.usage_value, "N/A");
        }
        return;
    }

    if (g_sysinfo_widgets.mem.usage_bar != NULL) {
        lv_bar_set_value(g_sysinfo_widgets.mem.usage_bar, (int)data->mem.usage_percent, LV_ANIM_ON);
        sysinfo_set_color_by_value(g_sysinfo_widgets.mem.usage_bar,
                                    g_sysinfo_widgets.mem.usage_value,
                                    data->mem.usage_percent);
    }
    if (g_sysinfo_widgets.mem.usage_value != NULL) {
        snprintf(temp_buf, sizeof(temp_buf), "%.1f%%", data->mem.usage_percent);
        lv_label_set_text(g_sysinfo_widgets.mem.usage_value, temp_buf);
    }
}

/**
 * @brief 更新GPU信息显示
 */
static void update_gpu_info(const sys_info_single_t *data)
{
    char temp_buf[64];

    /* 检查是否接收到有效数据 */
    if (!g_data_received) {
        /* 未接收到数据，显示 N/A */
        if (g_sysinfo_widgets.gpu.usage_bar != NULL) {
            lv_bar_set_value(g_sysinfo_widgets.gpu.usage_bar, 0, LV_ANIM_OFF);
        }
        if (g_sysinfo_widgets.gpu.usage_value != NULL) {
            lv_label_set_text(g_sysinfo_widgets.gpu.usage_value, "N/A");
        }
        if (g_sysinfo_widgets.gpu.temp_bar != NULL) {
            lv_bar_set_value(g_sysinfo_widgets.gpu.temp_bar, 0, LV_ANIM_OFF);
        }
        if (g_sysinfo_widgets.gpu.temp_value != NULL) {
            lv_label_set_text(g_sysinfo_widgets.gpu.temp_value, "N/A");
        }
        if (g_sysinfo_widgets.gpu.mem_bar != NULL) {
            lv_bar_set_value(g_sysinfo_widgets.gpu.mem_bar, 0, LV_ANIM_OFF);
        }
        if (g_sysinfo_widgets.gpu.mem_value != NULL) {
            lv_label_set_text(g_sysinfo_widgets.gpu.mem_value, "N/A");
        }
        return;
    }

    if (!data->gpu.has_gpu) {
        return;
    }

    /* GPU 使用率 */
    if (g_sysinfo_widgets.gpu.usage_bar != NULL) {
        lv_bar_set_value(g_sysinfo_widgets.gpu.usage_bar, (int)data->gpu.usage_percent, LV_ANIM_ON);
        sysinfo_set_color_by_value(g_sysinfo_widgets.gpu.usage_bar,
                                    g_sysinfo_widgets.gpu.usage_value,
                                    data->gpu.usage_percent);
    }
    if (g_sysinfo_widgets.gpu.usage_value != NULL) {
        snprintf(temp_buf, sizeof(temp_buf), "%.1f%%", data->gpu.usage_percent);
        lv_label_set_text(g_sysinfo_widgets.gpu.usage_value, temp_buf);
    }

    /* GPU 温度 */
    if (g_sysinfo_widgets.gpu.temp_bar != NULL) {
        lv_bar_set_value(g_sysinfo_widgets.gpu.temp_bar, (int)data->gpu.temperature, LV_ANIM_ON);
        sysinfo_set_color_by_value(g_sysinfo_widgets.gpu.temp_bar,
                                    g_sysinfo_widgets.gpu.temp_value,
                                    data->gpu.temperature);
    }
    if (g_sysinfo_widgets.gpu.temp_value != NULL) {
        snprintf(temp_buf, sizeof(temp_buf), "%.1f°C", data->gpu.temperature);
        lv_label_set_text(g_sysinfo_widgets.gpu.temp_value, temp_buf);
    }

    /* GPU 显存 */
    if (g_sysinfo_widgets.gpu.mem_bar != NULL) {
        lv_bar_set_value(g_sysinfo_widgets.gpu.mem_bar, (int)data->gpu.mem_usage_percent, LV_ANIM_ON);
        sysinfo_set_color_by_value(g_sysinfo_widgets.gpu.mem_bar,
                                    g_sysinfo_widgets.gpu.mem_value,
                                    data->gpu.mem_usage_percent);
    }
    if (g_sysinfo_widgets.gpu.mem_value != NULL) {
        snprintf(temp_buf, sizeof(temp_buf), "%.1f%%", data->gpu.mem_usage_percent);
        lv_label_set_text(g_sysinfo_widgets.gpu.mem_value, temp_buf);
    }
}

/**
 * @brief 更新网络信息显示
 */
static void update_net_info(const sys_info_single_t *data)
{
    if (g_sysinfo_widgets.net.upload_value != NULL) {
        lv_label_set_text(g_sysinfo_widgets.net.upload_value, data->net.upload_speed);
    }
    if (g_sysinfo_widgets.net.download_value != NULL) {
        lv_label_set_text(g_sysinfo_widgets.net.download_value, data->net.download_speed);
    }
}

/**
 * @brief 更新系统信息数据显示
 * @param data 系统信息数据指针
 * @note 此函数用于动态更新界面显示的数据，无需重新创建UI
 */
void sysinfo_update_display(const sys_info_single_t *data)
{
    if (data == NULL) {
        return;
    }

    update_device_info(data);
    update_cpu_info(data);
    update_mem_info(data);
    update_gpu_info(data);
    update_net_info(data);
}

/**
 * @brief UDP接收线程函数
 * @param arg 线程参数（未使用）
 * @return NULL
 */
static void *udp_receiver_thread(void *arg)
{
    int sock;
    struct sockaddr_in local_addr;
    sys_info_single_t recv_data;
    ssize_t recv_len;

    (void)arg; /* 未使用的参数 */

    printf("[UDP Receiver] 线程启动...\n");

    /* 创建UDP套接字 */
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        perror("[UDP Receiver] socket创建失败");
        return NULL;
    }

    /* 允许地址重用 */
    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("[UDP Receiver] setsockopt SO_REUSEADDR失败");
    }

    /* 绑定到广播端口 */
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(5005); /* 广播端口 */
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) {
        perror("[UDP Receiver] bind失败");
        close(sock);
        return NULL;
    }

    printf("[UDP Receiver] 绑定到端口 5005，等待数据...\n");

    /* 接收循环 */
    while (g_udp_running) {
        recv_len = recvfrom(sock, &recv_data, sizeof(sys_info_single_t), 0, NULL, NULL);

        if (recv_len == sizeof(sys_info_single_t)) {
            /* 使用互斥锁保护数据更新 */
            pthread_mutex_lock(&g_data_mutex);
            memcpy(&g_sysinfo_data, &recv_data, sizeof(sys_info_single_t));
            g_data_received = 1;  /* 标记已接收到数据 */
            g_last_recv_time = lv_tick_get();  /* 更新接收时间戳 */
            pthread_mutex_unlock(&g_data_mutex);

        } else if (recv_len < 0) {
            if (g_udp_running) {
                perror("[UDP Receiver] recvfrom失败");
            }
            break;
        }
    }

    close(sock);
    printf("[UDP Receiver] 线程退出\n");
    return NULL;
}

/**
 * @brief LVGL定时器回调函数（用于定期刷新界面）
 * @param timer LVGL定时器对象
 */
static void sysinfo_timer_callback(lv_timer_t *timer)
{
    (void)timer; /* 未使用的参数 */

    /* 使用互斥锁保护数据读取 */
    pthread_mutex_lock(&g_data_mutex);

    /* 检查数据是否超时（超过5秒未收到数据） */
    if (g_data_received) {
        uint32_t current_time = lv_tick_get();
        uint32_t elapsed = current_time - g_last_recv_time;

        if (elapsed > DATA_TIMEOUT_MS) {
            printf("[SysInfo] 数据超时 (%u ms)，切换为 N/A 显示\n", elapsed);
            g_data_received = 0;  /* 标记为无数据状态 */
        }
    }

    sysinfo_update_display(&g_sysinfo_data);
    pthread_mutex_unlock(&g_data_mutex);
}

/**
 * @brief 启动UDP接收线程
 * @return 0:成功, -1:失败
 */
int sysinfo_start_udp_receiver(void)
{
    if (g_udp_running) {
        printf("[UDP Receiver] 已经在运行中\n");
        return 0;
    }

    g_udp_running = 1;

    if (pthread_create(&g_udp_thread, NULL, udp_receiver_thread, NULL) != 0) {
        perror("[UDP Receiver] 线程创建失败");
        g_udp_running = 0;
        return -1;
    }

    printf("[UDP Receiver] 启动成功\n");
    return 0;
}

/**
 * @brief 停止UDP接收线程
 */
void sysinfo_stop_udp_receiver(void)
{
    if (!g_udp_running) {
        return;
    }

    printf("[UDP Receiver] 正在停止...\n");
    g_udp_running = 0;

    if (g_udp_thread != 0) {
        pthread_join(g_udp_thread, NULL);
        g_udp_thread = 0;
    }

    printf("[UDP Receiver] 已停止\n");
}

/**
 * @brief 启动LVGL定时器（用于定期刷新界面）
 * @param period_ms 刷新周期（毫秒）
 * @return LVGL定时器对象指针
 */
lv_timer_t *sysinfo_start_update_timer(uint32_t period_ms)
{
    lv_timer_t *timer = lv_timer_create(sysinfo_timer_callback, period_ms, NULL);
    if (timer == NULL) {
        printf("[LVGL Timer] 定时器创建失败\n");
        return NULL;
    }

    printf("[LVGL Timer] 定时器已启动，刷新周期: %u ms\n", period_ms);
    return timer;
}

