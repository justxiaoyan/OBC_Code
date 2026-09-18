/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "osal/src/nonos/Nonos_config.h"

/*
 * This table sets the default configurations for the DM Timers.
 * 
 * N.B. This platform supports 8 timer modules, each containing
 * two programmable timers (16 timers).
 */
TimerP_dmTimerDefault gDmTimerPInfoTbl[TimerP_numTimerDevices] = {
	/* Timer ID 0 */
	{
		"DMTimer0",
		0x0C008000,
		323,
	},
	/* Timer ID 1 */
	{
		"DMTimer1",
		0x0C008020,
		324,
	},
	/* Timer ID 2 */
	{
		"DMTimer2",
		0x0C009000,
		325,
	},
	/* Timer ID 3 */
	{
		"DMTimer3",
		0x0C009020,
		326,
	},
	/* Timer ID 4 */
	{
		"DMTimer4",
		0x0C00A000,
		327,
	},
	/* Timer ID 5 */
	{
		"DMTimer5",
		0x0C00A020,
		328,
	},
	/* Timer ID 6 */
	{
		"DMTimer6",
		0x0C00B000,
		329,
	},
	/* Timer ID 7 */
	{
		"DMTimer7",
		0x0C00B020,
		330,
	},
	/* Timer ID 8 */
	{
		"DMTimer8",
		0x0C00C000,
		331,
	},
	/* Timer ID 9 */
	{
		"DMTimer9",
		0x0C00C020,
		332,
	},
	/* Timer ID 10 */
	{
		"DMTimer10",
		0x0C00D000,
		333,
	},
	/* Timer ID 11 */
	{
		"DMTimer11",
		0x0C00D020,
		334,
	},
	/* Timer ID 12 */
	{
		"DMTimer12",
		0x0C00E000,
		335,
	},
	/* Timer ID 13 */
	{
		"DMTimer13",
		0x0C00E020,
		336,
	},
	/* Timer ID 14 */
	{
		"DMTimer14",
		0x0C00F000,
		337,
	},
	/* Timer ID 15 */
	{
		"DMTimer16",
		0x0C00F020,
		338,
	},
};

/* Returns the default frquency */
uint32_t TimerP_getDefaultFreq(uint32_t timerId)
{
    (void)timerId;
    /* Return the same default frequency for all timer Ids */
    return TIMERP_TIMER_FREQ;
}
