
#include "luckfox_720.h"
#include "net_date.h"
#include "sys_info.h"

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

        // 向左滑动 -> 切换到第二个界面
        if(dir == LV_DIR_LEFT) {
            if(current_scr == screen_main) {
                printf("[Debug] 执行跳转：Main -> Second\n");
                lv_scr_load_anim(screen_sysinfo, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
            }
        }
        // 向右滑动 -> 切换回主界面
        else if(dir == LV_DIR_RIGHT) {
            if(current_scr == screen_sysinfo) {
                printf("[Debug] 执行跳转：Second -> Main\n");
                lv_scr_load_anim(screen_main, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
            }
        }
    }
}


// 修改：主界面初始化
void lv_screem_main(void)
{
    screen_main = lv_scr_act();

    /* 1. 展示背景图片 */
    lv_display_bg(screen_main, "X:/mnt/nfs1/lvgl/guidao-720.bin");

    /* 2. 网络时间同步 */
    lv_net_sync();

    /* 3. 时间日期展示 */
    lv_time_display_init(screen_main);

    // 注册事件回调
    lv_obj_add_event_cb(screen_main, gesture_event_cb, LV_EVENT_GESTURE, NULL);

    /* 4. 加载主界面 */
    lv_scr_load(screen_main);
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

void lv_main_ayan(void)
{
    lv_screem_main();

    lv_screem_sysinfo();
}
