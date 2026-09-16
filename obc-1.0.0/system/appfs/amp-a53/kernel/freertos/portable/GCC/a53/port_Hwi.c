/*
 *  Copyright (C) 2021-2022 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

#include <stdint.h>		/* for uint32_t, uint64_t... */
#include <stddef.h>     /* for size_t */
#include "csl/arch/csl_arch.h"	/* for IntCommonHandler() */
//#include "drv/uart/UART_stdio.h"	/* for UART_printf() */

struct arm64_iframe {
	uint64_t r[30];
    uint64_t lr;
    uint64_t usp;
    uint64_t elr;
    uint64_t spsr;
};

typedef void (*exception_handler_t)(struct arm64_iframe *iframe, uint32_t esr);

__attribute__((__weak__)) exception_handler_t serror_handler = NULL;
__attribute__((__weak__)) exception_handler_t syncabort_handler = NULL;

__attribute__((section(".text.hwi"))) 
void HwiP_Sync_Abort_Handler(struct arm64_iframe *iframe, uint32_t esr)
{
    volatile uint32_t loop = 1U;

	if (syncabort_handler)
		syncabort_handler(iframe, esr);

	/* Go into an infinite loop.*/
    while (1U == loop)
	{
		;
    }
}

__attribute__((section(".text.hwi"))) 
void HwiP_SError_Handler(struct arm64_iframe *iframe, uint32_t esr)
{
    volatile uint32_t loop = 1U;

	if (serror_handler)
		serror_handler(iframe, esr);

	/* Go into an infinite loop. */	
    while (1U == loop)
    {
        ;
    }
}

/*
 * IRQ handler starts execution in FreeRTOS_IRQ_Handler, defined in portASM.S 
 * After some initial assembly logic it then branches to this function.
 * After exiting this function it does some more assembly including 
 * doing task switch if requested.
 */
void __attribute__((section(".text.hwi"))) HwiP_irq_handler(void)
{
	IntCommonHandler();
}
