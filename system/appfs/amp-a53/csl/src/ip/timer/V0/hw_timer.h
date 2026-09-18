/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */

/**
 * \file  hw_timer.h
 *
 * \brief Register-level header file for TIMER
 *
**/

#ifndef _CSLR_HW_TIMER_H_
#define _CSLR_HW_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif

#define TIMER_IRQSTATUS_RAW       (0x24U)
#define TIMER_IRQSTATUS           (0x28U)
#define TIMER_IRQENABLE_SET       (0x2CU)
#define TIMER_IRQENABLE_CLR       (0x30U)
#define TIMER_WAKEUP_EN           (0x34U)
#define TIMER_CONTROL             (0x38U) /* TCLR */
#define TIMER_VALUE               (0x3CU) /* TCRR */
#define TIMER_LOAD                (0x40U) /* TLDR */
#define TIMER_WRITE_PENDING       (0x48U) /* TWPS */
#define TIMER_RELOAD              TIMER_LOAD

#define TIMER_IRQ_OVERFLOW        (1U << 1)

/****************************************************************************************
* Field Definition Macros
*****************************************************************************************/
#define TIMER_LOAD_VALUE_SHIFT						(0U)
#define TIMER_LOAD_VALUE_MASK						(0xffffffffU)

#define TIMER_VALUE_COUNTER_SHIFT					(0U)
#define TIMER_VALUE_COUNTER_MASK					(0xffffffffU)

#define TIMER_CONTROL_ONESHOT_MODE_SHIFT			(1U)
#define TIMER_CONTROL_ONESHOT_MODE_MASK				(0x00000002U)

#define TIMER_CONTROL_TIMER_SIZE_SHIFT				(0U)
#define TIMER_CONTROL_TIMER_SIZE_MASK				(0x00000001U)
#define TIMER_CONTROL_TIMER_SIZE_VALUE_0			(0U)	/* 16-bit register */
#define TIMER_CONTROL_TIMER_SIZE_VALUE_1			(1U)	/* 32-bit register */

#define TIMER_CONTROL_TIMER_PRESCALER_SHIFT			(2U)
#define TIMER_CONTROL_TIMER_PRESCALER_MASK			(0x0000001cU)
#define TIMER_CONTROL_TIMER_PRESCALER_VALUE_0		(0U)
#define TIMER_CONTROL_TIMER_PRESCALER_VALUE_1		(1U)
#define TIMER_CONTROL_TIMER_PRESCALER_VALUE_2		(2U)
#define TIMER_CONTROL_TIMER_PRESCALER_VALUE_3		(3U)

#define TIMER_CONTROL_INTERRUPT_ENABLE_SHIFT		(5U)
#define TIMER_CONTROL_INTERRUPT_ENABLE_MASK			(0x00000020U)
#define TIMER_CONTROL_INTERRUPT_ENABLE_VALUE_0		(0U)
#define TIMER_CONTROL_INTERRUPT_ENABLE_VALUE_1		(1U)

#define TIMER_CONTROL_TIMER_MODE_SHIFT				(1U)
#define TIMER_CONTROL_TIMER_MODE_MASK				(0x00000002U)

#define TIMER_CONTROL_TIMER_ENABLE_SHIFT			(0U)
#define TIMER_CONTROL_TIMER_ENABLE_MASK				(0x00000001U)
#define TIMER_CONTROL_TIMER_ENABLE_VALUE_0      	(0U)
#define TIMER_CONTROL_TIMER_ENABLE_VALUE_1			(1U)

#define TIMER_RELOAD_VALUE_SHIFT					(0U)
#define TIMER_RELOAD_VALUE_MASK						(0xffffffffU)

#ifdef __cplusplus
}
#endif
#endif /* _CSLR_HW_TIMER_H_ */
