/*
 * main.c FreeRTOS Demo (A55)
 * 依赖：freertos_a55.a, libosal_a55.a, libcsl_a55.a
 */

#include <stdint.h>
#include "FreeRTOS.h"
#include "task.h"
#include "vs_uart.h"

// 定义写入地址
#define WRITE_ADDRESS (volatile uint32_t *)0x89000008
#define WRITE_ADDRESS1 (volatile uint32_t *)0x8900000C

/* -----------------------------------------------------------
 * 任务定义
 * ----------------------------------------------------------- */
void vMinimalTask1(void *pvParameters) {
    uint32_t counter = 0x76666666;

    while (1) {
        *WRITE_ADDRESS = counter++;

        vTaskDelay(1000);
        vs_printf("rtos task1 running\r\n");
    }

}

void vMinimalTask2(void *pvParameters) {
    uint32_t counter = 0x4234567;

    for (;;) {
        *WRITE_ADDRESS1 = counter++;

        vTaskDelay(2000);
        vs_printf("rtos task2 running\r\n");
    }
}

/* -----------------------------------------------------------
 * 主函数
 * ----------------------------------------------------------- */
int main(void)
{
    BaseType_t xReturn;

    vs_uart_init();

    vs_printf("ayan rtos app running\r\n");

    /* 2. 创建任务 */
    xReturn = xTaskCreate(
        vMinimalTask1,             /* 任务函数 */
        "Minimal1",                /* 名称 */
        2* 1024,                   /* 堆栈大小 (2K) */
        NULL,                      /* 参数 */
        3,                         /* 优先级*/
        NULL                       /* 句柄 */
    );

    if (xReturn != pdPASS) {
        while(1);
    }

    /* 2. 创建任务 */
    xReturn = xTaskCreate(
        vMinimalTask2,             /* 任务函数 */
        "Minimal2",                /* 名称 */
        2* 1024,                   /* 堆栈大小 (2K) */
        NULL,                      /* 参数 */
        3,                         /* 优先级*/
        NULL                       /* 句柄 */
    );

    if (xReturn != pdPASS) {
        while(1);
    }

    vTaskStartScheduler();

    while (1) {
        __asm__ volatile ("wfi");
    }

    return 0;
}



