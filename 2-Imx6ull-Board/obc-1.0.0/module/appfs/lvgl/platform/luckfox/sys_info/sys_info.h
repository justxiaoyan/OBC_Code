

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
#include <stdio.h> // 需要包含 stdio.h 以使用 printf


#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netdb.h>


extern lv_obj_t * screen_sysinfo; 

extern lv_obj_t * ui_Panel1;
extern lv_obj_t * ui_Panel3;
extern lv_obj_t * ui_Bar1;
extern lv_obj_t * ui_Label1;
extern lv_obj_t * ui_Label3;
extern lv_obj_t * ui_Bar3;
extern lv_obj_t * ui_Label4;
extern lv_obj_t * ui_Label5;
extern lv_obj_t * ui_Label6;
extern lv_obj_t * ui_Panel4;
extern lv_obj_t * ui_Label7;
extern lv_obj_t * ui_Label8;
extern lv_obj_t * ui_Bar4;
extern lv_obj_t * ui_Label9;
extern lv_obj_t * ui_Label10;
extern lv_obj_t * ui_Label11;
extern lv_obj_t * ui_Bar5;
extern lv_obj_t * ui_Bar6;
extern lv_obj_t * ui_Bar7;
extern lv_obj_t * ui_Panel5;
extern lv_obj_t * ui_Label12;
extern lv_obj_t * ui_Label13;
extern lv_obj_t * ui_Label14;
extern lv_obj_t * ui_Panel6;
extern lv_obj_t * ui_Label15;
extern lv_obj_t * ui_Label16;
extern lv_obj_t * ui_Label17;
extern lv_obj_t * ui_Label18;








extern void screen_sysinfo_screen_init(void);













#endif