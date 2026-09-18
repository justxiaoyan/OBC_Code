#include "tomato_timer.h"

/* 全局变量 */
lv_obj_t *screen_tomato = NULL;
tomato_config_t g_tomato_config = {
    .work_time = 25 * 60,           /* 25分钟工作 */
    .short_break_time = 5 * 60,     /* 5分钟短休息 */
    .long_break_time = 15 * 60,     /* 15分钟长休息 */
    .long_break_interval = 4        /* 4个番茄后长休息 */
};

tomato_data_t g_tomato_data = {
    .state = TOMATO_STATE_IDLE,
    .remaining_time = 25 * 60,
    .total_time = 25 * 60,
    .tomato_count = 0,
    .today_tomato_count = 0
};

tomato_ui_t g_tomato_ui = {0};
lv_timer_t *g_tomato_timer = NULL;

/* 前向声明 */
static void start_btn_event_cb(lv_event_t *e);
static void reset_btn_event_cb(lv_event_t *e);
static void settings_btn_event_cb(lv_event_t *e);
static void tomato_timer_cb(lv_timer_t *timer);
static void work_time_slider_cb(lv_event_t *e);
static void short_break_slider_cb(lv_event_t *e);
static void long_break_slider_cb(lv_event_t *e);
static void settings_close_cb(lv_event_t *e);

/**
 * @brief 获取状态文本
 */
static const char* get_state_text(tomato_state_t state)
{
    switch (state) {
        case TOMATO_STATE_IDLE:
            return "Ready";
        case TOMATO_STATE_WORKING:
            return "Working";
        case TOMATO_STATE_SHORT_BREAK:
            return "Short Break";
        case TOMATO_STATE_LONG_BREAK:
            return "Long Break";
        case TOMATO_STATE_PAUSED:
            return "Paused";
        default:
            return "Unknown";
    }
}

/**
 * @brief 获取状态颜色
 */
static lv_color_t get_state_color(tomato_state_t state)
{
    switch (state) {
        case TOMATO_STATE_WORKING:
            return lv_color_hex(0xE53935);  /* 红色 - 工作 */
        case TOMATO_STATE_SHORT_BREAK:
            return lv_color_hex(0x43A047);  /* 绿色 - 短休息 */
        case TOMATO_STATE_LONG_BREAK:
            return lv_color_hex(0x1E88E5);  /* 蓝色 - 长休息 */
        case TOMATO_STATE_PAUSED:
            return lv_color_hex(0xFB8C00);  /* 橙色 - 暂停 */
        default:
            return lv_color_hex(0x757575);  /* 灰色 - 空闲 */
    }
}

/**
 * @brief 更新界面显示
 */
void tomato_update_display(void)
{
    /* 更新时间显示 */
    uint32_t minutes = g_tomato_data.remaining_time / 60;
    uint32_t seconds = g_tomato_data.remaining_time % 60;
    lv_label_set_text_fmt(g_tomato_ui.time_label, "%02d:%02d", minutes, seconds);

    /* 更新圆形进度条 */
    uint32_t progress = 0;
    if (g_tomato_data.total_time > 0) {
        progress = (g_tomato_data.remaining_time * 100) / g_tomato_data.total_time;
    }
    lv_arc_set_value(g_tomato_ui.arc, progress);

    /* 更新进度条颜色 */
    lv_color_t color = get_state_color(g_tomato_data.state);
    lv_obj_set_style_arc_color(g_tomato_ui.arc, color, LV_PART_INDICATOR);

    /* 更新状态标签 */
    lv_label_set_text(g_tomato_ui.state_label, get_state_text(g_tomato_data.state));
    lv_obj_set_style_text_color(g_tomato_ui.state_label, color, 0);

    /* 更新番茄计数 */
    lv_label_set_text_fmt(g_tomato_ui.count_label, "🍅 Today: %d | Total: %d",
                          g_tomato_data.today_tomato_count,
                          g_tomato_data.tomato_count);

    /* 更新按钮文本 */
    if (g_tomato_data.state == TOMATO_STATE_IDLE) {
        lv_label_set_text(lv_obj_get_child(g_tomato_ui.start_btn, 0), "START");
    } else if (g_tomato_data.state == TOMATO_STATE_PAUSED) {
        lv_label_set_text(lv_obj_get_child(g_tomato_ui.start_btn, 0), "RESUME");
    } else {
        lv_label_set_text(lv_obj_get_child(g_tomato_ui.start_btn, 0), "PAUSE");
    }
}

/**
 * @brief 番茄时钟定时器回调（每秒触发）
 */
static void tomato_timer_cb(lv_timer_t *timer)
{
    /* 只在工作或休息状态下倒计时 */
    if (g_tomato_data.state != TOMATO_STATE_WORKING &&
        g_tomato_data.state != TOMATO_STATE_SHORT_BREAK &&
        g_tomato_data.state != TOMATO_STATE_LONG_BREAK) {
        return;
    }

    /* 倒计时 */
    if (g_tomato_data.remaining_time > 0) {
        g_tomato_data.remaining_time--;
        tomato_update_display();
    } else {
        /* 时间到 */
        if (g_tomato_data.state == TOMATO_STATE_WORKING) {
            /* 完成一个番茄 */
            g_tomato_data.tomato_count++;
            g_tomato_data.today_tomato_count++;
            printf("[Tomato] 完成一个番茄！总计：%d\n", g_tomato_data.tomato_count);

            /* 判断是短休息还是长休息 */
            if (g_tomato_data.tomato_count % g_tomato_config.long_break_interval == 0) {
                /* 长休息 */
                g_tomato_data.state = TOMATO_STATE_LONG_BREAK;
                g_tomato_data.remaining_time = g_tomato_config.long_break_time;
                g_tomato_data.total_time = g_tomato_config.long_break_time;
                printf("[Tomato] 开始长休息 15分钟\n");
            } else {
                /* 短休息 */
                g_tomato_data.state = TOMATO_STATE_SHORT_BREAK;
                g_tomato_data.remaining_time = g_tomato_config.short_break_time;
                g_tomato_data.total_time = g_tomato_config.short_break_time;
                printf("[Tomato] 开始短休息 5分钟\n");
            }
        } else {
            /* 休息结束，回到空闲状态 */
            g_tomato_data.state = TOMATO_STATE_IDLE;
            g_tomato_data.remaining_time = g_tomato_config.work_time;
            g_tomato_data.total_time = g_tomato_config.work_time;
            printf("[Tomato] 休息结束\n");
        }

        tomato_update_display();
    }
}

/**
 * @brief 启动番茄时钟
 */
void tomato_start(void)
{
    if (g_tomato_data.state == TOMATO_STATE_IDLE) {
        /* 从空闲状态开始工作 */
        g_tomato_data.state = TOMATO_STATE_WORKING;
        g_tomato_data.remaining_time = g_tomato_config.work_time;
        g_tomato_data.total_time = g_tomato_config.work_time;
        printf("[Tomato] 开始工作 25分钟\n");
    } else if (g_tomato_data.state == TOMATO_STATE_PAUSED) {
        /* 从暂停状态恢复 */
        if (g_tomato_data.remaining_time > 0) {
            /* 恢复到暂停前的状态 */
            if (g_tomato_data.total_time == g_tomato_config.work_time) {
                g_tomato_data.state = TOMATO_STATE_WORKING;
            } else if (g_tomato_data.total_time == g_tomato_config.short_break_time) {
                g_tomato_data.state = TOMATO_STATE_SHORT_BREAK;
            } else {
                g_tomato_data.state = TOMATO_STATE_LONG_BREAK;
            }
        }
        printf("[Tomato] 恢复计时\n");
    }

    /* 启动定时器 */
    if (g_tomato_timer == NULL) {
        g_tomato_timer = lv_timer_create(tomato_timer_cb, 1000, NULL);
    }

    tomato_update_display();
}

/**
 * @brief 暂停番茄时钟
 */
void tomato_pause(void)
{
    if (g_tomato_data.state == TOMATO_STATE_WORKING ||
        g_tomato_data.state == TOMATO_STATE_SHORT_BREAK ||
        g_tomato_data.state == TOMATO_STATE_LONG_BREAK) {
        g_tomato_data.state = TOMATO_STATE_PAUSED;
        printf("[Tomato] 暂停\n");
        tomato_update_display();
    }
}

/**
 * @brief 重置番茄时钟
 */
void tomato_reset(void)
{
    g_tomato_data.state = TOMATO_STATE_IDLE;
    g_tomato_data.remaining_time = g_tomato_config.work_time;
    g_tomato_data.total_time = g_tomato_config.work_time;
    printf("[Tomato] 重置\n");
    tomato_update_display();
}

/**
 * @brief 停止定时器
 */
void tomato_stop_timer(void)
{
    if (g_tomato_timer != NULL) {
        lv_timer_del(g_tomato_timer);
        g_tomato_timer = NULL;
    }
}

/**
 * @brief 开始/暂停按钮事件回调
 */
static void start_btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        if (g_tomato_data.state == TOMATO_STATE_IDLE ||
            g_tomato_data.state == TOMATO_STATE_PAUSED) {
            tomato_start();
        } else {
            tomato_pause();
        }
    }
}

/**
 * @brief 重置按钮事件回调
 */
static void reset_btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        tomato_reset();
    }
}

/**
 * @brief 设置按钮事件回调
 */
static void settings_btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        /* 显示设置面板 */
        if (g_tomato_ui.settings_panel != NULL) {
            lv_obj_clear_flag(g_tomato_ui.settings_panel, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/**
 * @brief 设置面板关闭回调
 */
static void settings_close_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        /* 隐藏设置面板 */
        if (g_tomato_ui.settings_panel != NULL) {
            lv_obj_add_flag(g_tomato_ui.settings_panel, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

/**
 * @brief 工作时间滑块回调
 */
static void work_time_slider_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);

    /* 更新配置（分钟转秒）*/
    g_tomato_config.work_time = value * 60;

    /* 如果当前是空闲状态，更新显示时间 */
    if (g_tomato_data.state == TOMATO_STATE_IDLE) {
        g_tomato_data.remaining_time = g_tomato_config.work_time;
        g_tomato_data.total_time = g_tomato_config.work_time;
        tomato_update_display();
    }

    /* 更新标签 */
    lv_obj_t *label = lv_event_get_user_data(e);
    lv_label_set_text_fmt(label, "Work: %d min", value);

    printf("[Tomato] 工作时间设置为：%d分钟\n", value);
}

/**
 * @brief 短休息时间滑块回调
 */
static void short_break_slider_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);

    g_tomato_config.short_break_time = value * 60;

    lv_obj_t *label = lv_event_get_user_data(e);
    lv_label_set_text_fmt(label, "Short Break: %d min", value);

    printf("[Tomato] 短休息时间设置为：%d分钟\n", value);
}

/**
 * @brief 长休息时间滑块回调
 */
static void long_break_slider_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);
    int32_t value = lv_slider_get_value(slider);

    g_tomato_config.long_break_time = value * 60;

    lv_obj_t *label = lv_event_get_user_data(e);
    lv_label_set_text_fmt(label, "Long Break: %d min", value);

    printf("[Tomato] 长休息时间设置为：%d分钟\n", value);
}

/**
 * @brief 初始化番茄时钟界面
 */
void screen_tomato_init(void)
{
    /* 创建屏幕 */
    screen_tomato = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_tomato, lv_color_hex(0x1A1A1A), 0);

    /* 创建圆形进度条 */
    g_tomato_ui.arc = lv_arc_create(screen_tomato);
    lv_obj_set_size(g_tomato_ui.arc, 450, 450);
    lv_obj_center(g_tomato_ui.arc);
    lv_arc_set_rotation(g_tomato_ui.arc, 270);
    lv_arc_set_bg_angles(g_tomato_ui.arc, 0, 360);
    lv_arc_set_value(g_tomato_ui.arc, 100);
    lv_obj_remove_style(g_tomato_ui.arc, NULL, LV_PART_KNOB);
    lv_obj_clear_flag(g_tomato_ui.arc, LV_OBJ_FLAG_CLICKABLE);

    /* 设置进度条样式 */
    lv_obj_set_style_arc_width(g_tomato_ui.arc, 20, LV_PART_MAIN);
    lv_obj_set_style_arc_width(g_tomato_ui.arc, 20, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(g_tomato_ui.arc, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_arc_color(g_tomato_ui.arc, lv_color_hex(0x757575), LV_PART_INDICATOR);

    /* 创建时间标签（中心大字体）*/
    g_tomato_ui.time_label = lv_label_create(screen_tomato);
    lv_label_set_text(g_tomato_ui.time_label, "25:00");
    lv_obj_set_style_text_font(g_tomato_ui.time_label, &lv_font_montserrat_30, 0);
    lv_obj_set_style_text_color(g_tomato_ui.time_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(g_tomato_ui.time_label, LV_ALIGN_CENTER, 0, 0);

    /* 创建状态标签 */
    g_tomato_ui.state_label = lv_label_create(screen_tomato);
    lv_label_set_text(g_tomato_ui.state_label, "Ready");
    lv_obj_set_style_text_font(g_tomato_ui.state_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(g_tomato_ui.state_label, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(g_tomato_ui.state_label, LV_ALIGN_CENTER, 0, -120);

    /* 创建番茄计数标签 */
    g_tomato_ui.count_label = lv_label_create(screen_tomato);
    lv_label_set_text(g_tomato_ui.count_label, "🍅 Today: 0 | Total: 0");
    lv_obj_set_style_text_font(g_tomato_ui.count_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(g_tomato_ui.count_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(g_tomato_ui.count_label, LV_ALIGN_CENTER, 0, 120);

    /* 创建开始/暂停按钮 */
    g_tomato_ui.start_btn = lv_btn_create(screen_tomato);
    lv_obj_set_size(g_tomato_ui.start_btn, 200, 60);
    lv_obj_align(g_tomato_ui.start_btn, LV_ALIGN_BOTTOM_MID, -110, -30);
    lv_obj_set_style_bg_color(g_tomato_ui.start_btn, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_radius(g_tomato_ui.start_btn, 30, 0);

    lv_obj_t *start_label = lv_label_create(g_tomato_ui.start_btn);
    lv_label_set_text(start_label, "START");
    lv_obj_set_style_text_font(start_label, &lv_font_montserrat_16, 0);
    lv_obj_center(start_label);

    lv_obj_add_event_cb(g_tomato_ui.start_btn, start_btn_event_cb, LV_EVENT_CLICKED, NULL);

    /* 创建重置按钮 */
    g_tomato_ui.reset_btn = lv_btn_create(screen_tomato);
    lv_obj_set_size(g_tomato_ui.reset_btn, 200, 60);
    lv_obj_align(g_tomato_ui.reset_btn, LV_ALIGN_BOTTOM_MID, 110, -30);
    lv_obj_set_style_bg_color(g_tomato_ui.reset_btn, lv_color_hex(0xF44336), 0);
    lv_obj_set_style_radius(g_tomato_ui.reset_btn, 30, 0);

    lv_obj_t *reset_label = lv_label_create(g_tomato_ui.reset_btn);
    lv_label_set_text(reset_label, "RESET");
    lv_obj_set_style_text_font(reset_label, &lv_font_montserrat_16, 0);
    lv_obj_center(reset_label);

    lv_obj_add_event_cb(g_tomato_ui.reset_btn, reset_btn_event_cb, LV_EVENT_CLICKED, NULL);

    /* 创建设置按钮（右上角小按钮）*/
    g_tomato_ui.settings_btn = lv_btn_create(screen_tomato);
    lv_obj_set_size(g_tomato_ui.settings_btn, 80, 50);
    lv_obj_align(g_tomato_ui.settings_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
    lv_obj_set_style_bg_color(g_tomato_ui.settings_btn, lv_color_hex(0x607D8B), 0);
    lv_obj_set_style_radius(g_tomato_ui.settings_btn, 10, 0);

    lv_obj_t *settings_label = lv_label_create(g_tomato_ui.settings_btn);
    lv_label_set_text(settings_label, "SET");
    lv_obj_set_style_text_font(settings_label, &lv_font_montserrat_14, 0);
    lv_obj_center(settings_label);

    lv_obj_add_event_cb(g_tomato_ui.settings_btn, settings_btn_event_cb, LV_EVENT_CLICKED, NULL);

    /* 创建设置面板（默认隐藏）*/
    g_tomato_ui.settings_panel = lv_obj_create(screen_tomato);
    lv_obj_set_size(g_tomato_ui.settings_panel, 600, 500);
    lv_obj_center(g_tomato_ui.settings_panel);
    lv_obj_set_style_bg_color(g_tomato_ui.settings_panel, lv_color_hex(0x263238), 0);
    lv_obj_set_style_border_color(g_tomato_ui.settings_panel, lv_color_hex(0x607D8B), 0);
    lv_obj_set_style_border_width(g_tomato_ui.settings_panel, 2, 0);
    lv_obj_set_style_radius(g_tomato_ui.settings_panel, 15, 0);
    lv_obj_add_flag(g_tomato_ui.settings_panel, LV_OBJ_FLAG_HIDDEN);

    /* 设置面板标题 */
    lv_obj_t *title = lv_label_create(g_tomato_ui.settings_panel);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    /* 工作时间滑块 */
    lv_obj_t *work_label = lv_label_create(g_tomato_ui.settings_panel);
    lv_label_set_text_fmt(work_label, "Work: %d min", g_tomato_config.work_time / 60);
    lv_obj_set_style_text_font(work_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(work_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(work_label, LV_ALIGN_TOP_LEFT, 30, 80);

    lv_obj_t *work_slider = lv_slider_create(g_tomato_ui.settings_panel);
    lv_obj_set_size(work_slider, 500, 20);
    lv_obj_align(work_slider, LV_ALIGN_TOP_LEFT, 50, 120);
    lv_slider_set_range(work_slider, 1, 60);
    lv_slider_set_value(work_slider, g_tomato_config.work_time / 60, LV_ANIM_OFF);
    lv_obj_add_event_cb(work_slider, work_time_slider_cb, LV_EVENT_VALUE_CHANGED, work_label);

    /* 短休息时间滑块 */
    lv_obj_t *short_break_label = lv_label_create(g_tomato_ui.settings_panel);
    lv_label_set_text_fmt(short_break_label, "Short Break: %d min", g_tomato_config.short_break_time / 60);
    lv_obj_set_style_text_font(short_break_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(short_break_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(short_break_label, LV_ALIGN_TOP_LEFT, 30, 180);

    lv_obj_t *short_break_slider = lv_slider_create(g_tomato_ui.settings_panel);
    lv_obj_set_size(short_break_slider, 500, 20);
    lv_obj_align(short_break_slider, LV_ALIGN_TOP_LEFT, 50, 220);
    lv_slider_set_range(short_break_slider, 1, 30);
    lv_slider_set_value(short_break_slider, g_tomato_config.short_break_time / 60, LV_ANIM_OFF);
    lv_obj_add_event_cb(short_break_slider, short_break_slider_cb, LV_EVENT_VALUE_CHANGED, short_break_label);

    /* 长休息时间滑块 */
    lv_obj_t *long_break_label = lv_label_create(g_tomato_ui.settings_panel);
    lv_label_set_text_fmt(long_break_label, "Long Break: %d min", g_tomato_config.long_break_time / 60);
    lv_obj_set_style_text_font(long_break_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(long_break_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align(long_break_label, LV_ALIGN_TOP_LEFT, 30, 280);

    lv_obj_t *long_break_slider = lv_slider_create(g_tomato_ui.settings_panel);
    lv_obj_set_size(long_break_slider, 500, 20);
    lv_obj_align(long_break_slider, LV_ALIGN_TOP_LEFT, 50, 320);
    lv_slider_set_range(long_break_slider, 10, 60);
    lv_slider_set_value(long_break_slider, g_tomato_config.long_break_time / 60, LV_ANIM_OFF);
    lv_obj_add_event_cb(long_break_slider, long_break_slider_cb, LV_EVENT_VALUE_CHANGED, long_break_label);

    /* 关闭按钮 */
    lv_obj_t *close_btn = lv_btn_create(g_tomato_ui.settings_panel);
    lv_obj_set_size(close_btn, 200, 50);
    lv_obj_align(close_btn, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0x607D8B), 0);

    lv_obj_t *close_label = lv_label_create(close_btn);
    lv_label_set_text(close_label, "CLOSE");
    lv_obj_set_style_text_font(close_label, &lv_font_montserrat_16, 0);
    lv_obj_center(close_label);

    lv_obj_add_event_cb(close_btn, settings_close_cb, LV_EVENT_CLICKED, NULL);

    printf("[Tomato] 番茄时钟界面初始化完成\n");

    /* 初始显示 */
    tomato_update_display();
}
