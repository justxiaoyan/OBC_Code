
#include "luckfox_720.h"
#include "date/net_date.h"
#include "sys_info/sys_info.h"
#include "stock/stock_info.h"
#include "menu/menu.h"

/* 1# 创建背景图片 */
void lv_display_bg(lv_obj_t * screen_main, char *path)
{
    lv_obj_t *bg_img = lv_img_create(screen_main);
    lv_img_set_src(bg_img, path);
    lv_obj_align(bg_img, LV_ALIGN_CENTER, 0, 0);
}


// --- 修改：加入调试打印的手势事件回调 ---
static void gesture_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    // 只有当手势事件发生时处理
    if(code == LV_EVENT_GESTURE) {

        // 2. 获取滑动方向
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());

        // 3. 打印方向值
        // LV_DIR_NONE=0, LEFT=1, RIGHT=2, TOP=4, BOTTOM=8

        lv_obj_t * current_scr = lv_scr_act(); // 获取当前屏幕

        // 向左滑动 -> 切换到下一个界面
        if(dir == LV_DIR_LEFT) {
            if(current_scr == screen_main) {
                printf("[Debug] 执行跳转：Main -> SysInfo\n");
                lv_scr_load_anim(screen_sysinfo, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
            } else if(current_scr == screen_sysinfo) {
                printf("[Debug] 执行跳转：SysInfo -> Stock\n");
                lv_scr_load_anim(screen_stock, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
            }
        }
        // 向右滑动 -> 切换到上一个界面
        else if(dir == LV_DIR_RIGHT) {
            if(current_scr == screen_stock) {
                printf("[Debug] 执行跳转：Stock -> SysInfo\n");
                lv_scr_load_anim(screen_sysinfo, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
            } else if(current_scr == screen_sysinfo) {
                printf("[Debug] 执行跳转：SysInfo -> Main\n");
                lv_scr_load_anim(screen_main, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
            } else if(current_scr == screen_main) {
                printf("[Debug] 执行跳转：Main -> Menu\n");
                lv_scr_load_anim(screen_menu, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
                /* 进入菜单时启动空闲定时器 */
                menu_start_idle_timer();
            }
        }
    }
}


// 修改：主界面初始化
void lv_screem_main(void)
{
    screen_main = lv_obj_create(NULL);

    /* 1. 展示背景图片 */
    lv_display_bg(screen_main, "X:/mnt/nfs1/lvgl/guidao-720.bin");

    /* 2. 网络时间同步 */
    lv_net_sync();

    /* 3. 时间日期展示 */
    lv_time_display_init(screen_main);

    // 注册事件回调
    lv_obj_add_event_cb(screen_main, gesture_event_cb, LV_EVENT_GESTURE, NULL);
}

// 系统信息界面
void lv_screem_sysinfo(void)
{
    // --- 初始化第二个界面 ---
    screen_sysinfo = lv_obj_create(NULL);

    lv_display_bg(screen_sysinfo, "X:/mnt/nfs1/lvgl/san2.bin");

    // 初始化系统信息界面UI
    screen_sysinfo_screen_init();

    // 启动UDP接收线程
    if (sysinfo_start_udp_receiver() == 0) {
        printf("[SysInfo] UDP接收线程启动成功\n");
    } else {
        printf("[SysInfo] UDP接收线程启动失败\n");
    }

    // 启动LVGL定时器，每500ms刷新一次界面
    lv_timer_t *timer = sysinfo_start_update_timer(500);
    if (timer != NULL) {
        printf("[SysInfo] 界面刷新定时器启动成功 (500ms)\n");
    } else {
        printf("[SysInfo] 界面刷新定时器启动失败\n");
    }

    // 注册事件回调
    lv_obj_add_event_cb(screen_sysinfo, gesture_event_cb, LV_EVENT_GESTURE, NULL);
}

// 股票信息界面
void lv_screem_stock(void)
{
    // --- 初始化第三个界面 ---
    screen_stock = lv_obj_create(NULL);

    lv_display_bg(screen_stock, "X:/mnt/nfs1/lvgl/san2.bin");

    // 加载股票配置
    if (stock_load_config() == 0) {
        printf("[Stock] 配置文件加载成功\n");
    } else {
        printf("[Stock] 配置文件加载失败\n");
    }

    // 初始化股票信息界面UI
    screen_stock_screen_init();

    // 启动股票数据更新线程
    if (stock_start_update_thread() == 0) {
        printf("[Stock] 数据更新线程启动成功\n");
    } else {
        printf("[Stock] 数据更新线程启动失败\n");
    }

    // 启动LVGL定时器，每1000ms刷新一次界面
    lv_timer_t *timer = lv_timer_create((lv_timer_cb_t)stock_update_display, 1000, NULL);
    if (timer != NULL) {
        printf("[Stock] 界面刷新定时器启动成功 (1000ms)\n");
    } else {
        printf("[Stock] 界面刷新定时器启动失败\n");
    }

    // 注册事件回调
    lv_obj_add_event_cb(screen_stock, gesture_event_cb, LV_EVENT_GESTURE, NULL);
}

// 菜单界面
void lv_screem_menu(void)
{
    // 初始化菜单界面
    screen_menu_init();

    // 注册手势事件回调（以便从菜单滑动回其他界面，如果需要）
    // lv_obj_add_event_cb(screen_menu, gesture_event_cb, LV_EVENT_GESTURE, NULL);
}

void lv_main_ayan(void)
{
    // 初始化所有界面
    lv_screem_main();
    lv_screem_sysinfo();
    lv_screem_stock();
    lv_screem_menu();

    // 默认加载菜单界面
    lv_scr_load(screen_menu);

    // 启动菜单空闲定时器
    menu_start_idle_timer();
}
