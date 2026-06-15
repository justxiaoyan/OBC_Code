
#ifndef __STOCK_INFO_H__
#define __STOCK_INFO_H__

#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
/* ==================== 配置常量 ==================== */
#define MAX_STOCKS 10              /* 最多监控的股票数量 */
#define BUFFER_SIZE 4096           /* HTTP响应缓冲区大小 */
#define CONFIG_FILE "/mnt/nfs1/lvgl/stock_config.ini"  /* 配置文件路径 */

/* ==================== 数据结构定义 ==================== */

/* 单个股票信息 */
typedef struct {
    char code[16];           /* 股票代码（如：sh600519） */
    char name[64];           /* 股票名称 */
    char description[64];    /* 股票描述（来自配置文件） */
    float current_price;     /* 当前价格 */
    float open_price;        /* 今日开盘价 */
    float yesterday_close;   /* 昨日收盘价 */
    float change_amount;     /* 涨跌额 */
    float change_percent;    /* 涨跌百分比 */
    char update_time[32];    /* 更新时间 */
    int valid;               /* 数据是否有效 */
} stock_info_t;

/* 股票界面UI控件 */
typedef struct {
    lv_obj_t *main_panel;           /* 主背景面板 */
    lv_obj_t *title_label;          /* 标题标签 */
    lv_obj_t *stock_panels[MAX_STOCKS];  /* 股票信息面板 */
    lv_obj_t *name_labels[MAX_STOCKS];   /* 股票名称标签 */
    lv_obj_t *price_labels[MAX_STOCKS];  /* 价格标签 */
    lv_obj_t *change_labels[MAX_STOCKS]; /* 涨跌幅标签 */
} stock_ui_widgets_t;

/* ==================== 全局变量声明 ==================== */

extern lv_obj_t *screen_stock;               /* 股票信息屏幕对象 */
extern stock_ui_widgets_t g_stock_widgets;   /* UI控件集合 */
extern stock_info_t g_stock_data[MAX_STOCKS];/* 股票信息数据 */
extern int g_stock_count;                     /* 当前股票数量 */

/* ==================== 函数声明 ==================== */

/**
 * @brief 初始化股票信息界面
 */
extern void screen_stock_screen_init(void);

/**
 * @brief 更新股票信息数据显示
 */
extern void stock_update_display(void);

/**
 * @brief 启动股票数据更新线程
 * @return 0:成功, -1:失败
 */
extern int stock_start_update_thread(void);

/**
 * @brief 停止股票数据更新线程
 */
extern void stock_stop_update_thread(void);

/**
 * @brief 从配置文件加载股票配置
 * @return 0:成功, -1:失败
 */
extern int stock_load_config(void);

/**
 * @brief 获取单个股票数据
 * @param stock_code 股票代码
 * @param info 股票信息结构指针
 * @return 0:成功, -1:失败
 */
extern int stock_fetch_data(const char *stock_code, stock_info_t *info);

#endif
