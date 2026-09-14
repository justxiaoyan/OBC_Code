/*
 *  Copyright (C) 2026 Hikvision Auto Technology Co., Ltd.
 *    Author: wsl <wangshulin@hikvision.com>
 */
/* Stolen from ti/osal/HwiP.h */


/** ============================================================================
 *  @file       HwiP.h
 *
 *  @brief      Hardware Interrupt module for the RTOS Porting Interface
 *
 *  The ::HwiP_disable/::HwiP_restore APIs can be called recursively. The order
 *  of the HwiP_restore calls, must be in reversed order. For example:
 *  @code
 *  uintptr_t key1, key2;
 *  key1 = HwiP_disable();
 *  key2 = HwiP_disable();
 *  HwiP_restore(key2);
 *  HwiP_restore(key1);
 *  @endcode
 *
 *  ============================================================================
 */

#ifndef _OSAL_HWIP_H_
#define _OSAL_HWIP_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
 * @def HWIP_USE_DEFAULT_PRIORITY
 * Use the default priority for the interrupts
 *
 *********************************************************************/
#define  HWIP_USE_DEFAULT_PRIORITY   (~((uint32_t)0))

/*!
 *  @brief    Opaque client reference to an instance of a HwiP
 *
 *  A HwiP_Handle returned from the ::HwiP_create represents that instance.
 */
typedef  void *HwiP_Handle;

/*!
 *  @brief    Status codes for HwiP APIs
 */
typedef enum HwiP_Status_e {
	HwiP_OK       = 0,
	HwiP_FAILURE  = (-(int32_t)1)
} HwiP_Status;

/**
 *  @brief Enumerates the types different trigger types.
 *  Please refer to Section 4.3.13 Interrupt Configuration Registers, GICD_ICFGRn
 *  of ARM Generic Interrupt Controller Architecture version 2.0
 *  Architecture Specification document for details.
 */
typedef enum {
	OSAL_GIC_INTR_TYPE_LEVEL = 1,
	/**< Corresponding interrupt is level sensitive */

	OSAL_GIC_INTR_TYPE_EDGE = 2,
	/**< Corresponding interrupt is edge sensitive */

	OSAL_GIC_INTR_TYPE_HIGH_LEVEL = 3,
	/**< Coressponding interrupt is high level sensitive */

	OSAL_GIC_INTR_TYPE_LOW_LEVEL = 4,
	/**< Coressponding interrupt is low level sensitive */

	OSAL_GIC_INTR_TYPE_RISING_EDGE = 5,
	/**< Coressponding interrupt is rising edge sensitive */

	OSAL_GIC_INTR_TYPE_FALLING_EDGE = 6
	/**< Coressponding interrupt is falling edge sensitive */
} OSAL_armGicTntrType_t;

/*!
 *  @brief  Prototype for the entry function for a hardware interrupt
 */	
typedef void (*HwiP_Fxn)(uintptr_t arg);

/*!
 *  @brief    Basic HwiP Parameters
 *
 *  Structure that contains the parameters passed into ::HwiP_create
 *  when creating a HwiP instance. The ::HwiP_Params_init function should
 *  be used to initialize the fields to default values before the application sets
 *  the fields manually. The HwiP default parameters are noted in
 *  HwiP_Params_init.
 */
typedef struct HwiP_Params_s {
	char      *name;      /*!< Name of the clock instance. Memory must
                               persist for the life of the clock instance.
                               This can be used for debugging purposes, or
                               set to NULL if not needed. */
	uintptr_t  arg;       /*!< Argument passed into the Hwi function. */
	uint32_t   intrPrio;  /*!< Device specific priority. 
                               Note: For R5 Core, baremetal case, there is no 
                               device specific priority set - all interrupts
                               are handled at the same priority. So, this is not
                               applicable for bare metal R5 Core interrupt config
                           */
	uint32_t   enableIntr; /*!< When set to TRUE, interrupt is enabled after the create
                                otherwise interrupt is disabled after HwiP_Create */
//	uint32_t   eventId;     /*!< Event Id associated */
	uint32_t   intrType;    /*!< Set an interrupt's trigger sensitivity for 
                                        ARM cortex-A Generic Interrupt Controller(GIC)
                                        v2.0 specific implementations as @ref OSAL_armGicTrigType_t 
                                    */
} HwiP_Params;

/*!
 *  @brief  Function to clear a single interrupt
 *
 *  @param  intrNum interrupt number to clear
 */
extern void HwiP_clearInterrupt(uint32_t intrNum);

/*!
 *  @brief  Function to create an interrupt on CortexM devices
 *
 *  @param  intrNum Interrupt Vector Id
 *
 *  @param  hwiFxn entry function of the hardware interrupt
 *
 *  @param  hwipParams    Pointer to the instance configuration parameters. NULL
 *                    denotes to use the default parameters. The HwiP default
 *                    parameters are noted in ::HwiP_Params_init.
 *
 *  @return 
 */
extern HwiP_Handle HwiP_create(uint32_t intrNum, HwiP_Fxn hwiFxn, const HwiP_Params *hwipParams);

#if 0
/*!
 *  @brief  Function to create an interrupt using VIM direct registration.
 *
 *  @param  intrNum Interrupt Vector Id
 *
 *  @param  hwiFxn entry function of the hardware interrupt
 *
 *  @param  hwipParams    Pointer to the instance configuration parameters. NULL
 *                    denotes to use the default parameters. The HwiP default
 *                    parameters are noted in ::HwiP_Params_init.
 */
extern HwiP_Handle HwiP_createDirect(uint32_t intrNum, HwiP_DirectFxn hwiFxn,
                                     const HwiP_Params *hwipParams);
#endif

/*!
 *  @brief  Function to delete an interrupt on CortexM devices
 *
 *  @param  handle returned from the HwiP_create call
 *
 *  @return A HwiP_Handle on success or a NULL
 */
extern HwiP_Status HwiP_delete(HwiP_Handle handle);

/*!
 *  @brief  Function to disable interrupts to enter a critical region
 *
 *  This function can be called multiple times.
 * 
 *  For Non-OS case, it must unwound in the reverse
 *  order. For example
 *  @code
 *  uintptr_t key1, key2;
 *  key1 = HwiP_disable();
 *  key2 = HwiP_disable();
 *  HwiP_restore(key2);
 *  HwiP_restore(key1);
 *  @endcode
 * 
 *  For FreeRTOS case, after the schedular had started the kernel keeps 
 *  track of critical nesting count.
 *  So here the return key will be NULL.
 *
 *  @return A key that must be passed to HwiP_restore to re-enable interrupts.
 */
extern uintptr_t HwiP_disable(void);

/*!
 *  @brief  Function to disable a single interrupt
 *
 *  @param  intrNum interrupt number to disable
 */
extern void HwiP_disableInterrupt(uint32_t intrNum);

/*!
 *  @brief  Function to enable a single interrupt
 *
 *  @param  intrNum interrupt number to enable
 */
extern void HwiP_enableInterrupt(uint32_t intrNum);

/*!
 *  @brief  Initialize params structure to default values.
 *
 *  The default parameters are:
 *   - name: NULL
 *   - arg: 0
 *   - priority: ~0
 *
 *  @param params  Pointer to the instance configuration parameters.
 */
extern void HwiP_Params_init(HwiP_Params *params);

/*!
 *  @brief  Function to restore interrupts to exit a critical region
 *
 *  @param  key return from HwiP_disable
 */
extern void HwiP_restore(uintptr_t key);

#if 0
/*!
 *  @brief  Function to get HwiP Handle from an interrupt number
 *
 *  @param  intrNum the interrupt number
 */
HwiP_Handle HwiP_getHandle(uint32_t intrNum);

/*!
 *  @brief  Function to get the eventId associated with an interrupt number
 *
 *  @param  intrNum the interrupt number
 */
int32_t HwiP_getEventId(uint32_t intrNum);
#endif

/*!
 *  @brief  Function to post the Hwi interrupt by software
 *
 *  @param  intrNum the interrupt number
 * 
 *  @return  osal_OK if successfull, osal_UNSUPPORTED if not supported by SOC 
 */
extern int32_t HwiP_post(uint32_t intrNum);

#ifdef __cplusplus
}
#endif
#endif /* _OSAL_HWIP_H_ */
