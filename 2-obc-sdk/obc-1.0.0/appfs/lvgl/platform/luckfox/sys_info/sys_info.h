

#ifndef __SYS_INFO_H__
#define __SYS_INFO_H__

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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netdb.h>

/* ==================== 数据结构定义 ==================== */

/* 基础设备信息 */
typedef struct {
    char device_name[64];    /* 设备名称 */
    char ip_address[32];     /* IP地址 */
} sys_info_base_t;

/* CPU信息 */
typedef struct {
    float usage_percent;     /* CPU使用率(%) */
    float temperature;       /* CPU温度(°C) */
    int core_count;          /* CPU核心数 */
} sys_info_cpu_t;

/* 内存信息 */
typedef struct {
    float usage_percent;     /* 内存占用率(%) */
} sys_info_mem_t;

/* GPU信息 */
typedef struct {
    int has_gpu;             /* 是否存在GPU (0:无, 1:有) */
    float usage_percent;     /* GPU使用率(%) */
    float temperature;       /* GPU温度(°C) */
    float mem_usage_percent; /* 显存占用率(%) */
} sys_info_gpu_t;

/* 网络信息 */
typedef struct {
    char upload_speed[32];   /* 上行带宽 */
    char download_speed[32]; /* 下行带宽 */
} sys_info_net_t;

/* 单个设备完整信息 */
typedef struct {
    sys_info_base_t base;    /* 基础信息 */
    sys_info_cpu_t cpu;      /* CPU信息 */
    sys_info_mem_t mem;      /* 内存信息 */
    sys_info_gpu_t gpu;      /* GPU信息 */
    sys_info_net_t net;      /* 网络信息 */
} sys_info_single_t;

/* ==================== UI控件组织结构 ==================== */

/* CPU相关UI控件 */
typedef struct {
    lv_obj_t *panel;           /* CPU信息面板 */
    lv_obj_t *usage_bar;       /* CPU使用率进度条 */
    lv_obj_t *usage_label;     /* CPU使用率标签 */
    lv_obj_t *usage_value;     /* CPU使用率数值 */
    lv_obj_t *temp_bar;        /* CPU温度进度条 */
    lv_obj_t *temp_label;      /* CPU温度标签 */
    lv_obj_t *temp_value;      /* CPU温度数值 */
} sysinfo_ui_cpu_t;

/* 内存相关UI控件 */
typedef struct {
    lv_obj_t *panel;           /* 内存信息面板 */
    lv_obj_t *usage_bar;       /* 内存使用率进度条 */
    lv_obj_t *usage_label;     /* 内存使用率标签 */
    lv_obj_t *usage_value;     /* 内存使用率数值 */
} sysinfo_ui_mem_t;

/* GPU相关UI控件 */
typedef struct {
    lv_obj_t *panel;           /* GPU信息面板 */
    lv_obj_t *usage_bar;       /* GPU使用率进度条 */
    lv_obj_t *usage_label;     /* GPU使用率标签 */
    lv_obj_t *usage_value;     /* GPU使用率数值 */
    lv_obj_t *temp_bar;        /* GPU温度进度条 */
    lv_obj_t *temp_label;      /* GPU温度标签 */
    lv_obj_t *temp_value;      /* GPU温度数值 */
    lv_obj_t *mem_bar;         /* GPU显存进度条 */
    lv_obj_t *mem_label;       /* GPU显存标签 */
    lv_obj_t *mem_value;       /* GPU显存数值 */
} sysinfo_ui_gpu_t;

/* 网络相关UI控件 */
typedef struct {
    lv_obj_t *panel;           /* 网络信息面板 */
    lv_obj_t *upload_label;    /* 上行标签 */
    lv_obj_t *upload_value;    /* 上行数值 */
    lv_obj_t *download_label;  /* 下行标签 */
    lv_obj_t *download_value;  /* 下行数值 */
} sysinfo_ui_net_t;

/* 系统信息UI总控件 */
typedef struct {
    lv_obj_t *main_panel;       /* 主背景面板 */
    lv_obj_t *device_info;      /* 设备信息标签（IP+名称） */
    lv_obj_t *device_indicator; /* 设备指示器标签（显示 "设备 1/3"） */
    sysinfo_ui_cpu_t cpu;       /* CPU UI控件组 */
    sysinfo_ui_mem_t mem;       /* 内存UI控件组 */
    sysinfo_ui_gpu_t gpu;       /* GPU UI控件组 */
    sysinfo_ui_net_t net;       /* 网络UI控件组 */
} sysinfo_ui_widgets_t;

/* ==================== 全局变量声明 ==================== */

extern lv_obj_t *screen_sysinfo;               /* 系统信息屏幕对象 */
extern sysinfo_ui_widgets_t g_sysinfo_widgets; /* UI控件集合 */
extern sys_info_single_t g_sysinfo_data;       /* 系统信息数据 */




/* ==================== 函数声明 ==================== */

/**
 * @brief 初始化系统信息界面
 */
extern void screen_sysinfo_screen_init(void);

/**
 * @brief 更新系统信息数据显示
 * @param data 系统信息数据指针
 */
extern void sysinfo_update_display(const sys_info_single_t *data);

/**
 * @brief 启动UDP接收线程（接收系统监控广播）
 * @return 0:成功, -1:失败
 */
extern int sysinfo_start_udp_receiver(void);

/**
 * @brief 停止UDP接收线程
 */
extern void sysinfo_stop_udp_receiver(void);

/**
 * @brief 启动LVGL定时器（用于定期刷新界面）
 * @param period_ms 刷新周期（毫秒），建议500-1000ms
 * @return LVGL定时器对象指针
 */
extern lv_timer_t *sysinfo_start_update_timer(uint32_t period_ms);

/**
 * @brief 根据数值设置进度条和文本颜色
 * @param bar 进度条对象
 * @param label 文本标签对象
 * @param value 当前数值
 */
extern void sysinfo_set_color_by_value(lv_obj_t *bar, lv_obj_t *label, float value);










#endif