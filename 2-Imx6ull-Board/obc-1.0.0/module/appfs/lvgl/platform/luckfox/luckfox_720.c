
#include "luckfox_720.h"

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
void lv_time_display_init(void)
{

    /* --- 1. 创建“时:分”标签 --- */
    label_time = lv_label_create(lv_scr_act());
    lv_label_set_recolor(label_time, true);
    lv_label_set_text(label_time, "#87e2f0 00:00#");
    lv_obj_set_style_text_font(label_time, &lv_font_number_200, 0);

    // LV_OPA_COVER (255) 是不透明，LV_OPA_TRANSP (0) 是完全透明
    // LV_OPA_90 表示 90% 不透明度 (稍微有点透)
    lv_obj_set_style_opa(label_time, LV_OPA_90, 0);
    // 设置显示位置
    lv_obj_align(label_time, LV_ALIGN_CENTER, -60, 180);

    /* --- 2. 创建“秒”标签 --- */
    label_sec = lv_label_create(lv_scr_act());
    lv_label_set_recolor(label_sec, true);
    lv_label_set_text(label_sec, "#dcdbe2 :00#");
    lv_obj_set_style_text_font(label_sec, &lv_font_number_100, 0);
    // 这里设置稍微更透明一点，形成对比，或者保持一致均可
    lv_obj_set_style_opa(label_sec, LV_OPA_80, 0);
    // 位置设置
    lv_obj_align_to(label_sec, label_time, LV_ALIGN_OUT_RIGHT_BOTTOM, 0, 0);

    /* --- 3. 创建“日期”标签 --- */
    label_date = lv_label_create(lv_scr_act());
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

void lv_display_bg(void)
{
    /* 1# 创建背景图片 */
    char *bg_image_path = "X:/mnt/nfs1/lvgl/guidao-720.bin";

    // 3. 文件存在，现在可以安全地创建图片对象了
    lv_obj_t *bg_img = lv_img_create(lv_scr_act());
    lv_img_set_src(bg_img, bg_image_path);
    lv_obj_align(bg_img, LV_ALIGN_CENTER, 0, 0);
}

void lv_main_ayan(void)
{
    /* 展示背景图片 */
    lv_display_bg();

    /* 网络时间同步 */
    lv_net_sync();

    /* 时间日期展示 */
    lv_time_display_init();
}
