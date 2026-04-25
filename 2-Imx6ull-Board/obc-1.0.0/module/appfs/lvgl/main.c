#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include "lv_drivers/display/fbdev.h"
#include "lv_drivers/indev/evdev.h"
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h> // 需要包含 stdio.h 以使用 printf

#define DISP_BUF_SIZE (720 * 720)


/* 全局变量：标签对象指针 */
static lv_obj_t *label_time; // 显示 时:分
static lv_obj_t *label_sec;  // 显示 秒
static lv_obj_t *label_date; // 新增：日期标签

/**
 * @brief 时间更新回调函数
 */
static void time_update_timer_cb(lv_timer_t * timer)
{
    time_t now;
    struct tm timeinfo;
    char time_buf[64]; 
    char sec_buf[16];
    char date_buf[64]; // 新增：日期缓冲区

    // 1. 获取当前时间
    time(&now); 
    localtime_r(&now, &timeinfo);

    // 2. 格式化 时:分
    snprintf(time_buf, sizeof(time_buf), 
             "#87e2f0 %02d##dcdbe2 :##87e2f0 %02d#", 
             timeinfo.tm_hour, 
             timeinfo.tm_min);

    // 3. 格式化 秒
    snprintf(sec_buf, sizeof(sec_buf), 
             "#dcdbe2 :%02d#", 
             timeinfo.tm_sec);

    // --- 修改部分：格式化日期 (添加纯白颜色代码) ---
    // #ffffff 代表纯白色
    strftime(date_buf, sizeof(date_buf), "#FFFFFF %Y/%m/%d##FFFFFF  %A#", &timeinfo);

    // 4. 更新标签文本
    if (label_time != NULL) {
        lv_label_set_text(label_time, time_buf);
    }
    if (label_sec != NULL) {
        lv_label_set_text(label_sec, sec_buf);
    }
    // --- 新增：更新日期 ---
    if (label_date != NULL) {
        lv_label_set_text(label_date, date_buf);
    }
}

/**
 * @brief 初始化时间显示界面
 */
void lv_time_display_init(void)
{
    /* --- 1. 创建“时:分”标签 --- */
    label_time = lv_label_create(lv_scr_act());
    lv_label_set_recolor(label_time, true); 
    lv_label_set_text(label_time, "#87e2f0 00:00#"); 
    lv_obj_set_style_text_font(label_time, &lv_font_number_200, 0); 
    
    // --- 新增：设置透明度 ---
    // LV_OPA_COVER (255) 是不透明，LV_OPA_TRANSP (0) 是完全透明
    // LV_OPA_90 表示 90% 不透明度 (稍微有点透)
    lv_obj_set_style_opa(label_time, LV_OPA_90, 0); 
    
    lv_obj_align(label_time, LV_ALIGN_CENTER, -60, 180); 

    /* --- 2. 创建“秒”标签 --- */
    label_sec = lv_label_create(lv_scr_act());
    lv_label_set_recolor(label_sec, true); 
    lv_label_set_text(label_sec, "#dcdbe2 :00#"); 
    lv_obj_set_style_text_font(label_sec, &lv_font_number_100, 0); 
    
    // --- 新增：设置透明度 ---
    // 这里设置稍微更透明一点，形成对比，或者保持一致均可
    lv_obj_set_style_opa(label_sec, LV_OPA_80, 0); 

    // 位置设置
    lv_obj_align_to(label_sec, label_time, LV_ALIGN_OUT_RIGHT_BOTTOM, 0, 0); 


    /* --- 3. 创建“日期”标签 (新增部分) --- */
    label_date = lv_label_create(lv_scr_act());
    lv_label_set_recolor(label_date, true); 
    // 日期不需要重着色，使用默认颜色即可
    lv_label_set_text(label_date, "#FFFFFF 2026/04/25 Sunday#"); // 设置初始文本
    lv_obj_set_style_text_font(label_date, &lv_font_AZ_40, 0); 
    // 将日期标签放置在“时:分”标签的正上方
    lv_obj_align_to(label_date, label_time, LV_ALIGN_OUT_BOTTOM_MID, 60, 20);
    // 这里设置稍微更透明一点，形成对比，或者保持一致均可
    lv_obj_set_style_opa(label_date, LV_OPA_80, 0); 
    /* --- 3. 创建定时器 --- */
    lv_timer_create(time_update_timer_cb, 1000, NULL);
}
uint8_t net_map[] =   { /* 0X00,0X01,0X28,0X00,0X28,0X00, */
0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
0X00,0X00,0X00,0X00,0X00,0X03,0XFF,0XE0,0X00,0X00,0X07,0XFF,0XF0,0X00,0X00,0X07,
0XFF,0XF0,0X00,0X00,0X07,0X00,0X70,0X00,0X00,0X07,0X00,0X70,0X00,0X00,0X07,0X00,
0X70,0X00,0X00,0X07,0X00,0X70,0X00,0X00,0X07,0X00,0X70,0X00,0X00,0X07,0X00,0X70,
0X00,0X00,0X07,0XFF,0XF0,0X00,0X00,0X07,0XFF,0XF0,0X00,0X00,0X03,0XFF,0XE0,0X00,
0X00,0X00,0X1C,0X00,0X00,0X00,0X00,0X1C,0X00,0X00,0X0F,0XFF,0XFF,0XFF,0XF8,0X1F,
0XFF,0XFF,0XFF,0XF8,0X1F,0XFF,0XFF,0XFF,0XF8,0X00,0X30,0X00,0X06,0X00,0X00,0X30,
0X00,0X06,0X00,0X00,0X38,0X00,0X0E,0X00,0X0F,0XFF,0XC1,0XFF,0XF8,0X0F,0XFF,0XC3,
0XFF,0XF8,0X0E,0X00,0XC3,0X80,0X38,0X0E,0X00,0XC3,0X80,0X38,0X0E,0X00,0XC3,0X80,
0X38,0X0E,0X00,0XC3,0X80,0X38,0X0E,0X00,0XC3,0X80,0X38,0X0E,0X00,0XC3,0X80,0X38,
0X0E,0X00,0XC3,0X80,0X38,0X0F,0XFF,0XC3,0XFF,0XF8,0X0F,0XFF,0XC1,0XFF,0XF0,0X00,
0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,
0X00,0X00,0X00,0X00,0X00,0X00,0X00,0X00,};

const lv_img_dsc_t net = {
  .header.cf = LV_IMG_CF_ALPHA_1BIT,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = 40,
  .header.h = 40,
  .data_size = 200,
  .data = net_map,
};


void lv_net_img(void)
{
    lv_obj_t * img_icon = lv_img_create(lv_scr_act());
    lv_img_set_src(img_icon, &net);
    lv_obj_align(img_icon, LV_ALIGN_TOP_RIGHT, 0, 0);

    // --- 关键部分 ---
// 1. 设置想要染成的颜色 (例如：红色 #f2daf3)
    lv_obj_set_style_img_recolor(img_icon, lv_color_hex(0xf2daf3), 0);

    // 2. 设置染色的不透明度 (必须设为 COVER，否则颜色会很淡或者不显示)
    lv_obj_set_style_img_recolor_opa(img_icon, LV_OPA_COVER, 0);

    // 1. 确保背景是透明的（虽然 ALPHA_1BIT 默认就是透明的，但为了保险可以显式设置）
    //lv_obj_set_style_bg_opa(img_icon, LV_OPA_TRANSP, 0);

    // 2. 【不要】设置 img_recolor！
    // 如果你设置了 img_recolor 为白色，或者设置了 recolor_opa，
    // 可能会导致显示异常或者多余的计算。
    // 保持默认，它就是纯白色的。
}

void lv_demo_ayan(void)
{
    lv_fs_file_t file;
    lv_fs_res_t res;
    char * image_path = "X:/mnt/nfs1/lvgl/guidao-720.bin";

    // 3. 文件存在，现在可以安全地创建图片对象了
    lv_obj_t * img = lv_img_create(lv_scr_act());
    lv_img_set_src(img, image_path);
    lv_obj_align(img, LV_ALIGN_CENTER, 0, 0);

    lv_net_img();

    lv_time_display_init();

    /* 显示一个lable，里面显示时间 */

    // /* ----------- 全局变量 ----------- */
    // static lv_obj_t *label_time;

    // /* 创建标签 */
    // label_time = lv_label_create(lv_scr_act());
    // lv_label_set_recolor(label_time, true);
    // lv_label_set_text(label_time, "#cbd8da 00:00#");
    // lv_obj_set_style_text_font(label_time, &lv_font_number_200, 0);
    // lv_obj_align(label_time, LV_ALIGN_CENTER, 0, 180);
}

int main(void)
{
    /*LittlevGL init*/
    lv_init();

    /*Linux frame buffer device init*/
    fbdev_init();

    /*A small buffer for LittlevGL to draw the screen's content*/
    static lv_color_t buf[DISP_BUF_SIZE];

    /*Initialize a descriptor for the buffer*/
    static lv_disp_draw_buf_t disp_buf;
    lv_disp_draw_buf_init(&disp_buf, buf, NULL, DISP_BUF_SIZE);

    /*Initialize and register a display driver*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf   = &disp_buf;
    disp_drv.flush_cb   = fbdev_flush;
    disp_drv.hor_res    = 720;
    disp_drv.ver_res    = 720;
    disp_drv.sw_rotate = 1;                     // 启用软件旋转
    disp_drv.rotated = LV_DISP_ROT_NONE;          // 设置旋转角度：90/180/270 度
    lv_disp_drv_register(&disp_drv);

    evdev_init();
    static lv_indev_drv_t indev_drv_1;
    lv_indev_drv_init(&indev_drv_1); /*Basic initialization*/
    indev_drv_1.type = LV_INDEV_TYPE_POINTER;

    /*This function will be called periodically (by the library) to get the mouse position and state*/
   indev_drv_1.read_cb = evdev_read;
    lv_indev_t *mouse_indev = lv_indev_drv_register(&indev_drv_1);


    /*Set a cursor for the mouse*/
  //  LV_IMG_DECLARE(mouse_cursor_icon)
  //  lv_obj_t * cursor_obj = lv_img_create(lv_scr_act()); /*Create an image object for the cursor */
  //  lv_img_set_src(cursor_obj, &mouse_cursor_icon);           /*Set the image source*/
   // lv_indev_set_cursor(mouse_indev, cursor_obj);             /*Connect the image  object to the driver*/

   lv_fs_posix_init();


    /*Create a Demo*/
    //lv_demo_widgets();
    //lv_demo_benchmark();
    //lv_demo_stress();
    //lv_demo_music();
    /*Handle LitlevGL tasks (tickless mode)*/

    lv_demo_ayan();

    while(1) {
        lv_timer_handler();
        usleep(5000);
    }

    return 0;
}

/*Set in lv_conf.h as `LV_TICK_CUSTOM_SYS_TIME_EXPR`*/
uint32_t custom_tick_get(void)
{
    static uint64_t start_ms = 0;
    if(start_ms == 0) {
        struct timeval tv_start;
        gettimeofday(&tv_start, NULL);
        start_ms = (tv_start.tv_sec * 1000000 + tv_start.tv_usec) / 1000;
    }

    struct timeval tv_now;
    gettimeofday(&tv_now, NULL);
    uint64_t now_ms;
    now_ms = (tv_now.tv_sec * 1000000 + tv_now.tv_usec) / 1000;

    uint32_t time_ms = now_ms - start_ms;
    return time_ms;
}
