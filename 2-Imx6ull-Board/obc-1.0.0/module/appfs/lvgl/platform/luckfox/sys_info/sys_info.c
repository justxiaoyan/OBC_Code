
#include "sys_info.h"






lv_obj_t * ui_Panel1 = NULL;
lv_obj_t * ui_Panel3 = NULL;
lv_obj_t * ui_Bar1 = NULL;
lv_obj_t * ui_Label1 = NULL;
lv_obj_t * ui_Label3 = NULL;
lv_obj_t * ui_Bar3 = NULL;
lv_obj_t * ui_Label4 = NULL;
lv_obj_t * ui_Label5 = NULL;
lv_obj_t * ui_Label6 = NULL;
lv_obj_t * ui_Panel4 = NULL;
lv_obj_t * ui_Label7 = NULL;
lv_obj_t * ui_Label8 = NULL;
lv_obj_t * ui_Bar4 = NULL;
lv_obj_t * ui_Label9 = NULL;
lv_obj_t * ui_Label10 = NULL;
lv_obj_t * ui_Label11 = NULL;
lv_obj_t * ui_Bar5 = NULL;
lv_obj_t * ui_Bar6 = NULL;
lv_obj_t * ui_Bar7 = NULL;
lv_obj_t * ui_Panel5 = NULL;
lv_obj_t * ui_Label12 = NULL;
lv_obj_t * ui_Label13 = NULL;
lv_obj_t * ui_Label14 = NULL;
lv_obj_t * ui_Panel6 = NULL;
lv_obj_t * ui_Label15 = NULL;
lv_obj_t * ui_Label16 = NULL;
lv_obj_t * ui_Label17 = NULL;
lv_obj_t * ui_Label18 = NULL;
// event funtions


lv_obj_t * screen_sysinfo;  // 第二个界面（新增）

// build funtions

void screen_sysinfo_screen_init(void)
{
    //screen_sysinfo = lv_obj_create(NULL);
    //lv_obj_clear_flag(screen_sysinfo, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    //lv_obj_set_style_bg_img_src(screen_sysinfo, &ui_img_san2_png, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Panel1 = lv_obj_create(screen_sysinfo);
    lv_obj_set_width(ui_Panel1, 286);
    lv_obj_set_height(ui_Panel1, 648);
    lv_obj_set_x(ui_Panel1, -190);
    lv_obj_set_y(ui_Panel1, -2);
    lv_obj_set_align(ui_Panel1, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Panel1, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_opa(ui_Panel1, 25, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Panel3 = lv_obj_create(screen_sysinfo);
    lv_obj_set_width(ui_Panel3, 246);
    lv_obj_set_height(ui_Panel3, 137);
    lv_obj_set_x(ui_Panel3, -192);
    lv_obj_set_y(ui_Panel3, -204);
    lv_obj_set_align(ui_Panel3, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Panel3, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_opa(ui_Panel3, 25, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Bar1 = lv_bar_create(screen_sysinfo);
    lv_bar_set_value(ui_Bar1, 6, LV_ANIM_OFF);
    lv_bar_set_start_value(ui_Bar1, 0, LV_ANIM_OFF);
    lv_obj_set_width(ui_Bar1, 221);
    lv_obj_set_height(ui_Bar1, 15);
    lv_obj_set_x(ui_Bar1, -192);
    lv_obj_set_y(ui_Bar1, -224);
    lv_obj_set_align(ui_Bar1, LV_ALIGN_CENTER);

    ui_Label1 = lv_label_create(screen_sysinfo);
    lv_obj_set_width(ui_Label1, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label1, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label1, -268);
    lv_obj_set_y(ui_Label1, -248);
    lv_obj_set_align(ui_Label1, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label1, "CPU Used");
    lv_obj_set_style_text_color(ui_Label1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label3 = lv_label_create(screen_sysinfo);
    lv_obj_set_width(ui_Label3, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label3, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label3, -194);
    lv_obj_set_y(ui_Label3, -298);
    lv_obj_set_align(ui_Label3, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label3, "10.10.0.56         ayan-server");
    lv_obj_set_style_text_color(ui_Label3, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label3, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Label3, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Bar3 = lv_bar_create(screen_sysinfo);
    lv_bar_set_value(ui_Bar3, 53, LV_ANIM_OFF);
    lv_bar_set_start_value(ui_Bar3, 0, LV_ANIM_OFF);
    lv_obj_set_width(ui_Bar3, 221);
    lv_obj_set_height(ui_Bar3, 15);
    lv_obj_set_x(ui_Bar3, -193);
    lv_obj_set_y(ui_Bar3, -160);
    lv_obj_set_align(ui_Bar3, LV_ALIGN_CENTER);

    lv_obj_set_style_bg_color(ui_Bar3, lv_color_hex(0xDEF246), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_Bar3, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    ui_Label4 = lv_label_create(screen_sysinfo);
    lv_obj_set_width(ui_Label4, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label4, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label4, -263);
    lv_obj_set_y(ui_Label4, -185);
    lv_obj_set_align(ui_Label4, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label4, "CPU Temp");
    lv_obj_set_style_text_color(ui_Label4, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label4, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label5 = lv_label_create(screen_sysinfo);
    lv_obj_set_width(ui_Label5, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label5, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label5, -108);
    lv_obj_set_y(ui_Label5, -182);
    lv_obj_set_align(ui_Label5, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label5, "53.0°C");
    lv_obj_set_style_text_color(ui_Label5, lv_color_hex(0xEFF75A), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label5, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label6 = lv_label_create(screen_sysinfo);
    lv_obj_set_width(ui_Label6, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label6, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label6, -101);
    lv_obj_set_y(ui_Label6, -247);
    lv_obj_set_align(ui_Label6, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label6, "6.3%");
    lv_obj_set_style_text_color(ui_Label6, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label6, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Panel4 = lv_obj_create(screen_sysinfo);
    lv_obj_set_width(ui_Panel4, 246);
    lv_obj_set_height(ui_Panel4, 76);
    lv_obj_set_x(ui_Panel4, -190);
    lv_obj_set_y(ui_Panel4, -76);
    lv_obj_set_align(ui_Panel4, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Panel4, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_opa(ui_Panel4, 25, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label7 = lv_label_create(screen_sysinfo);
    lv_obj_set_width(ui_Label7, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label7, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label7, -265);
    lv_obj_set_y(ui_Label7, -92);
    lv_obj_set_align(ui_Label7, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label7, "MEM Used");
    lv_obj_set_style_text_color(ui_Label7, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label7, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label8 = lv_label_create(screen_sysinfo);
    lv_obj_set_width(ui_Label8, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label8, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label8, -110);
    lv_obj_set_y(ui_Label8, -91);
    lv_obj_set_align(ui_Label8, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label8, "31.6%");
    lv_obj_set_style_text_color(ui_Label8, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label8, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Bar4 = lv_bar_create(screen_sysinfo);
    lv_bar_set_value(ui_Bar4, 31, LV_ANIM_OFF);
    lv_bar_set_start_value(ui_Bar4, 0, LV_ANIM_OFF);
    lv_obj_set_width(ui_Bar4, 221);
    lv_obj_set_height(ui_Bar4, 15);
    lv_obj_set_x(ui_Bar4, -193);
    lv_obj_set_y(ui_Bar4, -66);
    lv_obj_set_align(ui_Bar4, LV_ALIGN_CENTER);

    ui_Label9 = lv_label_create(screen_sysinfo);
    lv_obj_set_width(ui_Label9, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label9, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label9, -103);
    lv_obj_set_y(ui_Label9, 13);
    lv_obj_set_align(ui_Label9, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label9, "0%");
    lv_obj_set_style_text_color(ui_Label9, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label9, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label10 = lv_label_create(screen_sysinfo);
    lv_obj_set_width(ui_Label10, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label10, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label10, -105);
    lv_obj_set_y(ui_Label10, 67);
    lv_obj_set_align(ui_Label10, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label10, "43.0°C");
    lv_obj_set_style_text_color(ui_Label10, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label10, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label11 = lv_label_create(screen_sysinfo);
    lv_obj_set_width(ui_Label11, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label11, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label11, -98);
    lv_obj_set_y(ui_Label11, 124);
    lv_obj_set_align(ui_Label11, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label11, "0.6%");
    lv_obj_set_style_text_color(ui_Label11, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label11, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Bar5 = lv_bar_create(screen_sysinfo);
    lv_bar_set_value(ui_Bar5, 5, LV_ANIM_OFF);
    lv_bar_set_start_value(ui_Bar5, 0, LV_ANIM_OFF);
    lv_obj_set_width(ui_Bar5, 221);
    lv_obj_set_height(ui_Bar5, 15);
    lv_obj_set_x(ui_Bar5, -188);
    lv_obj_set_y(ui_Bar5, 39);
    lv_obj_set_align(ui_Bar5, LV_ALIGN_CENTER);

    ui_Bar6 = lv_bar_create(screen_sysinfo);
    lv_bar_set_value(ui_Bar6, 43, LV_ANIM_OFF);
    lv_bar_set_start_value(ui_Bar6, 0, LV_ANIM_OFF);
    lv_obj_set_width(ui_Bar6, 221);
    lv_obj_set_height(ui_Bar6, 15);
    lv_obj_set_x(ui_Bar6, -189);
    lv_obj_set_y(ui_Bar6, 96);
    lv_obj_set_align(ui_Bar6, LV_ALIGN_CENTER);

    ui_Bar7 = lv_bar_create(screen_sysinfo);
    lv_bar_set_value(ui_Bar7, 5, LV_ANIM_OFF);
    lv_bar_set_start_value(ui_Bar7, 0, LV_ANIM_OFF);
    lv_obj_set_width(ui_Bar7, 221);
    lv_obj_set_height(ui_Bar7, 15);
    lv_obj_set_x(ui_Bar7, -187);
    lv_obj_set_y(ui_Bar7, 147);
    lv_obj_set_align(ui_Bar7, LV_ALIGN_CENTER);

    ui_Panel5 = lv_obj_create(screen_sysinfo);
    lv_obj_set_width(ui_Panel5, 246);
    lv_obj_set_height(ui_Panel5, 184);
    lv_obj_set_x(ui_Panel5, -189);
    lv_obj_set_y(ui_Panel5, 82);
    lv_obj_set_align(ui_Panel5, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Panel5, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_opa(ui_Panel5, 25, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label12 = lv_label_create(screen_sysinfo);
    lv_obj_set_width(ui_Label12, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label12, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label12, -258);
    lv_obj_set_y(ui_Label12, 17);
    lv_obj_set_align(ui_Label12, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label12, "GPU Used");
    lv_obj_set_style_text_color(ui_Label12, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label12, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label13 = lv_label_create(screen_sysinfo);
    lv_obj_set_width(ui_Label13, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label13, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label13, -259);
    lv_obj_set_y(ui_Label13, 73);
    lv_obj_set_align(ui_Label13, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label13, "GPU Temp");
    lv_obj_set_style_text_color(ui_Label13, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label13, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label14 = lv_label_create(screen_sysinfo);
    lv_obj_set_width(ui_Label14, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label14, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label14, -262);
    lv_obj_set_y(ui_Label14, 126);
    lv_obj_set_align(ui_Label14, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label14, "GPU MEM");
    lv_obj_set_style_text_color(ui_Label14, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label14, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Panel6 = lv_obj_create(screen_sysinfo);
    lv_obj_set_width(ui_Panel6, 246);
    lv_obj_set_height(ui_Panel6, 103);
    lv_obj_set_x(ui_Panel6, -186);
    lv_obj_set_y(ui_Panel6, 251);
    lv_obj_set_align(ui_Panel6, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_Panel6, LV_OBJ_FLAG_SCROLLABLE);      /// Flags
    lv_obj_set_style_opa(ui_Panel6, 25, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label15 = lv_label_create(screen_sysinfo);
    lv_obj_set_width(ui_Label15, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label15, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label15, -253);
    lv_obj_set_y(ui_Label15, 229);
    lv_obj_set_align(ui_Label15, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label15, "Net Up");
    lv_obj_set_style_text_color(ui_Label15, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label15, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label16 = lv_label_create(screen_sysinfo);
    lv_obj_set_width(ui_Label16, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label16, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label16, -127);
    lv_obj_set_y(ui_Label16, 229);
    lv_obj_set_align(ui_Label16, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label16, "Net Down");
    lv_obj_set_style_text_color(ui_Label16, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label16, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Label16, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label17 = lv_label_create(screen_sysinfo);
    lv_obj_set_width(ui_Label17, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label17, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label17, -128);
    lv_obj_set_y(ui_Label17, 263);
    lv_obj_set_align(ui_Label17, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label17, "165.9KB/s");
    lv_obj_set_style_text_color(ui_Label17, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label17, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Label17, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_Label18 = lv_label_create(screen_sysinfo);
    lv_obj_set_width(ui_Label18, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_height(ui_Label18, LV_SIZE_CONTENT);    /// 1
    lv_obj_set_x(ui_Label18, -252);
    lv_obj_set_y(ui_Label18, 263);
    lv_obj_set_align(ui_Label18, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label18, "2.4MB/s");
    lv_obj_set_style_text_color(ui_Label18, lv_color_hex(0xF0F35F), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label18, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Label18, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

}






