
#include "luckfox_720.h"
#include "net_date.h"

lv_obj_t * screen_sysinfo;  // 第二个界面（新增）

/* 1# 创建背景图片 */
void lv_display_bg(lv_obj_t * screen_main, char *path)
{
    lv_obj_t *bg_img = lv_img_create(screen_main);
    lv_img_set_src(bg_img, path);
    lv_obj_align(bg_img, LV_ALIGN_CENTER, 0, 0);
}



// --- 新增：第二个界面的初始化函数 ---
void lv_display_second_screen(lv_obj_t * scr)
{
    // 这里简单创建一个背景色和标签，你可以替换成你的图片
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x222222), 0); // 深灰色背景
    
    lv_obj_t * label = lv_label_create(scr);
    lv_label_set_text(label, "这是第二个界面\n向左/右滑动返回");
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
}

// --- 修改：加入调试打印的手势事件回调 ---
static void gesture_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    
    // 1. 打印事件类型，确认回调是否被调用
    printf("👀 [Debug] 事件代码: %d (LV_EVENT_GESTURE 应该是 %d)\n", code, LV_EVENT_GESTURE);

    // 只有当手势事件发生时处理
    if(code == LV_EVENT_GESTURE) {
        
        // 2. 获取滑动方向
        lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
        
        // 3. 打印方向值
        // LV_DIR_NONE=0, LEFT=1, RIGHT=2, TOP=4, BOTTOM=8
        printf("👆 [Debug] 检测到手势方向值: %d\n", dir);

        lv_obj_t * current_scr = lv_scr_act(); // 获取当前屏幕
        printf("ℹ️ [Debug] 当前屏幕指针: %p\n", current_scr);

        // 向左滑动 -> 切换到第二个界面
        if(dir == LV_DIR_LEFT) {
            printf("⬅️ [Debug] 判定：向左滑动！\n");
            if(current_scr == screen_main) {
                printf("🚀 [Debug] 执行跳转：Main -> Second\n");
                lv_scr_load_anim(screen_sysinfo, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
            }
        }
        // 向右滑动 -> 切换回主界面
        else if(dir == LV_DIR_RIGHT) {
            printf("➡️ [Debug] 判定：向右滑动！\n");
            if(current_scr == screen_sysinfo) {
                printf("🚀 [Debug] 执行跳转：Second -> Main\n");
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

    lv_display_bg(screen_sysinfo, "X:/mnt/nfs1/lvgl/san.bin");

    lv_display_second_screen(screen_sysinfo);

    // 注册事件回调
    lv_obj_add_event_cb(screen_sysinfo, gesture_event_cb, LV_EVENT_GESTURE, NULL);
}

void lv_main_ayan(void)
{
    lv_screem_main();

    lv_screem_sysinfo();
}
