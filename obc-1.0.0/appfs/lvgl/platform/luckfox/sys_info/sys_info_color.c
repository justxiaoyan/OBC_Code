/**
 * @file sys_info_color.c
 * @brief 系统信息颜色设置辅助函数（独立编译以规避编译器bug）
 */

#include "sys_info.h"

/* 颜色配置表 */
static const uint32_t bar_colors[3] = {0x2196F3, 0xDEF246, 0xF44336};
static const uint32_t txt_colors[3] = {0xFFFFFF, 0xEFF75A, 0xFF5555};

/**
 * @brief 根据数值获取颜色索引（使用整数运算规避编译器浮点bug）
 */
static int get_color_index(float value)
{
    int val = (int)value;

    if (val < 30) return 0;
    if (val < 70) return 1;
    return 2;
}

/**
 * @brief 根据数值设置进度条和文本颜色
 */
void sysinfo_set_color_by_value(lv_obj_t *bar, lv_obj_t *label, float value)
{
    int idx;

    idx = get_color_index(value);

    if (bar) {
        lv_obj_set_style_bg_color(bar, lv_color_hex(bar_colors[idx]),
                                   LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }

    if (label) {
        lv_obj_set_style_text_color(label, lv_color_hex(txt_colors[idx]),
                                     LV_PART_MAIN | LV_STATE_DEFAULT);
    }
}
