/* Minimal AM62x FreeRTOS AMP demo for A53 core 3. */
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#define AM62X_DEMO_COUNTER_ADDRESS ((volatile uint32_t *)0x8f000000U)

static inline void am62x_flush_counter(void)
{
	__asm__ volatile("dc cvac, %0" : : "r"(AM62X_DEMO_COUNTER_ADDRESS) : "memory");
	__asm__ volatile("dsb sy" ::: "memory");
}

static void am62x_counter_task(void *argument)
{
	(void)argument;

	for (;;) {
		uint32_t value = *AM62X_DEMO_COUNTER_ADDRESS;
		*AM62X_DEMO_COUNTER_ADDRESS = value + 5U;
		//am62x_flush_counter();
		vTaskDelay(pdMS_TO_TICKS(1000U));
	}
}

int main(void)
{
	/* Make the first observation deterministic after each rproc start. */
	*AM62X_DEMO_COUNTER_ADDRESS = 0U;
	am62x_flush_counter();

	if (xTaskCreate(am62x_counter_task, "am62x-counter", 1024U, NULL,
					(configMAX_PRIORITIES - 2U), NULL) != pdPASS) {
		for (;;) {
			__asm__ volatile("wfe");
		}
	}

	vTaskStartScheduler();
	for (;;) {
		__asm__ volatile("wfe");
	}
}
