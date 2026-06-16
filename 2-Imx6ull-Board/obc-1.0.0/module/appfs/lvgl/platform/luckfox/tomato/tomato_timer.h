#ifndef __TOMATO_TIMER_H__
#define __TOMATO_TIMER_H__

#include "lvgl/lvgl.h"
#include <stdio.h>
#include <time.h>

/* 番茄时钟状态枚举 */
typedef enum {
    TOMATO_STATE_IDLE = 0,      /* 空闲状态 */
    TOMATO_STATE_WORKING,       /* 工作中 */
    TOMATO_STATE_SHORT_BREAK,   /* 短休息 */
    TOMATO_STATE_LONG_BREAK,    /* 长休息 */
    TOMATO_STATE_PAUSED         /* 暂停 */
} tomato_state_t;

/* 番茄时钟配置 */
typedef struct {
    uint32_t work_time;         /* 工作时间（秒），默认25分钟 */
    uint32_t short_break_time;  /* 短休息时间（秒），默认5分钟 */
    uint32_t long_break_time;   /* 长休息时间（秒），默认15分钟 */
    uint32_t long_break_interval; /* 长休息间隔（完成几个番茄后）*/
} tomato_config_t;

/* 番茄时钟数据 */
typedef struct {
    tomato_state_t state;       /* 当前状态 */
    uint32_t remaining_time;    /* 剩余时间（秒）*/
    uint32_t total_time;        /* 总时间（秒）*/
    uint32_t tomato_count;      /* 完成的番茄数 */
    uint32_t today_tomato_count; /* 今日完成的番茄数 */
} tomato_data_t;

/* UI控件 */
typedef struct {
    lv_obj_t *arc;              /* 圆形进度条 */
    lv_obj_t *time_label;       /* 时间标签（分:秒）*/
    lv_obj_t *state_label;      /* 状态标签 */
    lv_obj_t *count_label;      /* 番茄计数标签 */
    lv_obj_t *start_btn;        /* 开始/暂停按钮 */
    lv_obj_t *reset_btn;        /* 重置按钮 */
    lv_obj_t *settings_btn;     /* 设置按钮 */
    lv_obj_t *settings_panel;   /* 设置面板 */
} tomato_ui_t;

/* 全局变量声明 */
extern lv_obj_t *screen_tomato;
extern tomato_config_t g_tomato_config;
extern tomato_data_t g_tomato_data;
extern tomato_ui_t g_tomato_ui;
extern lv_timer_t *g_tomato_timer;

/* 函数声明 */

/**
 * @brief 初始化番茄时钟界面
 */
void screen_tomato_init(void);

/**
 * @brief 启动番茄时钟
 */
void tomato_start(void);

/**
 * @brief 暂停番茄时钟
 */
void tomato_pause(void);

/**
 * @brief 重置番茄时钟
 */
void tomato_reset(void);

/**
 * @brief 更新界面显示
 */
void tomato_update_display(void);

/**
 * @brief 停止番茄时钟定时器
 */
void tomato_stop_timer(void);

#endif /* __TOMATO_TIMER_H__ */
