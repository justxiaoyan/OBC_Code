#ifndef __MENU_H__
#define __MENU_H__

#include "lvgl/lvgl.h"
#include <stdio.h>

/* 菜单类型枚举 */
typedef enum {
    MENU_TIME = 0,      /* 时间界面 */
    MENU_SYSINFO,       /* 系统信息界面 */
    MENU_STOCK,         /* 股票界面 */
    MENU_TOMATO,        /* 番茄时钟界面 */
    MENU_MAX
} menu_type_t;

/* 全局变量声明 */
extern lv_obj_t *screen_menu;           /* 菜单屏幕对象 */
extern lv_timer_t *g_menu_idle_timer;   /* 菜单空闲定时器 */

/**
 * @brief 初始化菜单界面
 */
void screen_menu_init(void);

/**
 * @brief 重置菜单空闲定时器（用户有操作时调用）
 */
void menu_reset_idle_timer(void);

/**
 * @brief 停止菜单空闲定时器
 */
void menu_stop_idle_timer(void);

/**
 * @brief 启动菜单空闲定时器
 */
void menu_start_idle_timer(void);

#endif /* __MENU_H__ */
