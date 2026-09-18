/* AM62x main_timer1 (AM654-compatible DMTimer V2). main_timer0 belongs to U-Boot. */
#include <stdint.h>
#include "osal/src/nonos/Nonos_config.h"

TimerP_dmTimerDefault gDmTimerPInfoTbl[TimerP_numTimerDevices] = {
	{
		"main_timer1",
		0x02410000U,
		153U, /* DTS GIC_SPI 121 plus the architectural SPI base (32). */
	},
};

uint32_t TimerP_getDefaultFreq(uint32_t timerId)
{
	(void)timerId;
	return TIMERP_TIMER_FREQ;
}
