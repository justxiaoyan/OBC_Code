
#include "lvgl/lvgl.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <netdb.h>

extern lv_obj_t * screen_main;
extern lv_obj_t * screen_sysinfo;
extern lv_obj_t * screen_stock;
extern lv_obj_t * screen_menu;

void lv_main_ayan(void);

