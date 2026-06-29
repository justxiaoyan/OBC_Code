#include "menu.h"
#include "../date/net_date.h"
#include "../sys_info/sys_info.h"
#include "../stock/stock_info.h"
#include "../tomato/tomato_timer.h"

/* 全局变量 */
lv_obj_t *screen_menu = NULL;
lv_timer_t *g_menu_idle_timer = NULL;

/* 菜单项对象 */
static lv_obj_t *menu_panels[MENU_MAX] = {NULL};

/* 外部屏幕声明 */
extern lv_obj_t *screen_main;
extern lv_obj_t *screen_sysinfo;
extern lv_obj_t *screen_stock;
extern lv_obj_t *screen_tomato;

/**
 * @brief 菜单空闲定时器回调（20秒无操作自动跳转到时间界面）
 */
static void menu_idle_timer_cb(lv_timer_t *timer)
{
    printf("[Menu] 20秒无操作，自动跳转到时间界面\n");

    /* 停止定时器 */
    menu_stop_idle_timer();

    /* 跳转到时间界面 */
    lv_scr_load_anim(screen_main, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, false);
}

/**
 * @brief 菜单项点击事件回调
 */
static void menu_item_clicked_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        menu_type_t *menu_type = (menu_type_t *)lv_event_get_user_data(e);

        if (menu_type == NULL) {
            printf("[Menu] 错误：菜单类型为空\n");
            return;
        }

        /* 停止空闲定时器 */
        menu_stop_idle_timer();

        printf("[Menu] 点击菜单项：%d\n", *menu_type);

        /* 根据菜单类型跳转到对应界面 */
        switch (*menu_type) {
            case MENU_TIME:
                printf("[Menu] 跳转到时间界面\n");
                lv_scr_load_anim(screen_main, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, false);
                break;

            case MENU_SYSINFO:
                printf("[Menu] 跳转到系统信息界面\n");
                lv_scr_load_anim(screen_sysinfo, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, false);
                break;

            case MENU_STOCK:
                printf("[Menu] 跳转到股票界面\n");
                lv_scr_load_anim(screen_stock, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, false);
                break;

            case MENU_TOMATO:
                printf("[Menu] 跳转到番茄时钟界面\n");
                lv_scr_load_anim(screen_tomato, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, false);
                break;

            default:
                printf("[Menu] 未知菜单类型\n");
                break;
        }
    }
}

/**
 * @brief 创建单个菜单项
 * @param parent 父对象
 * @param x X坐标
 * @param y Y坐标
 * @param width 宽度
 * @param height 高度
 * @param text 显示文本
 * @param menu_type 菜单类型
 * @return 创建的面板对象
 */
static lv_obj_t *create_menu_item(lv_obj_t *parent, lv_coord_t x, lv_coord_t y,
                                   lv_coord_t width, lv_coord_t height,
                                   const char *text, menu_type_t menu_type)
{
    /* 创建面板 */
    lv_obj_t *panel = lv_obj_create(parent);
    lv_obj_set_size(panel, width, height);
    lv_obj_set_pos(panel, x, y);

    /* 设置样式 */
    lv_obj_set_style_radius(panel, 10, 0);
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_bg_opa(panel, LV_OPA_80, 0);
    lv_obj_set_style_border_width(panel, 2, 0);
    lv_obj_set_style_border_color(panel, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_shadow_width(panel, 10, 0);
    lv_obj_set_style_shadow_spread(panel, 2, 0);

    /* 添加按下效果 */
    lv_obj_set_style_bg_color(panel, lv_color_hex(0x1976D2), LV_STATE_PRESSED);
    lv_obj_set_style_shadow_width(panel, 5, LV_STATE_PRESSED);

    /* 启用点击标志 */
    lv_obj_add_flag(panel, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

    /* 创建文本标签 */
    lv_obj_t *label = lv_label_create(panel);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_30, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(label);

    /* 分配内存保存菜单类型（注意：需要静态或动态分配） */
    static menu_type_t menu_types[MENU_MAX];
    menu_types[menu_type] = menu_type;

    /* 注册点击事件 */
    lv_obj_add_event_cb(panel, menu_item_clicked_cb, LV_EVENT_CLICKED, &menu_types[menu_type]);

    return panel;
}

/**
 * @brief 初始化菜单界面
 */
void screen_menu_init(void)
{
    /* 创建菜单屏幕 */
    screen_menu = lv_obj_create(NULL);

    /* 设置背景颜色 */
    lv_obj_set_style_bg_color(screen_menu, lv_color_hex(0x000000), 0);

    /* 计算菜单项尺寸（720x720分为4块，每块360x360，留出间隙） */
    const lv_coord_t item_width = 350;
    const lv_coord_t item_height = 350;
    const lv_coord_t gap = 10;
    const lv_coord_t start_x = (720 - item_width * 2 - gap) / 2;
    const lv_coord_t start_y = (720 - item_height * 2 - gap) / 2;

    /* 创建4个菜单项 */
    /* 左上：时间 */
    menu_panels[MENU_TIME] = create_menu_item(screen_menu,
                                               start_x, start_y,
                                               item_width, item_height,
                                               "Time", MENU_TIME);

    /* 右上：系统信息 */
    menu_panels[MENU_SYSINFO] = create_menu_item(screen_menu,
                                                   start_x + item_width + gap, start_y,
                                                   item_width, item_height,
                                                   "SysInfo", MENU_SYSINFO);

    /* 左下：股票 */
    menu_panels[MENU_STOCK] = create_menu_item(screen_menu,
                                                start_x, start_y + item_height + gap,
                                                item_width, item_height,
                                                "Stock", MENU_STOCK);

    /* 右下：番茄时钟 */
    menu_panels[MENU_TOMATO] = create_menu_item(screen_menu,
                                                 start_x + item_width + gap, start_y + item_height + gap,
                                                 item_width, item_height,
                                                 "Tomato", MENU_TOMATO);

    /* 番茄时钟现在可用，设置为橙色 */
    lv_obj_set_style_bg_color(menu_panels[MENU_TOMATO], lv_color_hex(0xFF9800), 0);
    lv_obj_set_style_bg_opa(menu_panels[MENU_TOMATO], LV_OPA_80, 0);

    printf("[Menu] 菜单界面初始化完成\n");
}

/**
 * @brief 重置菜单空闲定时器
 */
void menu_reset_idle_timer(void)
{
    if (g_menu_idle_timer != NULL) {
        lv_timer_reset(g_menu_idle_timer);
        printf("[Menu] 空闲定时器已重置\n");
    }
}

/**
 * @brief 停止菜单空闲定时器
 */
void menu_stop_idle_timer(void)
{
    if (g_menu_idle_timer != NULL) {
        lv_timer_del(g_menu_idle_timer);
        g_menu_idle_timer = NULL;
        printf("[Menu] 空闲定时器已停止\n");
    }
}

/**
 * @brief 启动菜单空闲定时器（20秒）
 */
void menu_start_idle_timer(void)
{
    /* 如果已存在，先删除 */
    menu_stop_idle_timer();

    /* 创建新定时器，20秒后触发，只触发一次 */
    g_menu_idle_timer = lv_timer_create(menu_idle_timer_cb, 20000, NULL);
    lv_timer_set_repeat_count(g_menu_idle_timer, 1);

    printf("[Menu] 空闲定时器已启动（20秒）\n");
}
