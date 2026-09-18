/* AM62x A53 OSAL resource and clock settings. */
#ifndef _OSAL_SOC_AM62X_H_
#define _OSAL_SOC_AM62X_H_

#ifdef __cplusplus
extern "C" {
#endif

#define TIMERP_TIMER_FREQ                 ((uint32_t)25000000U)
#define TimerP_numTimerDevices            ((uint32_t)1U)
#define TimerP_ANY_MASK                   ((uint32_t)0x1U)
#define TimerP_AVAILABLE_MASK             ((uint32_t)0x1U)

#define OSAL_NONOS_MAX_HWIP_PER_SOC       ((uint32_t)64U)
#define OSAL_NONOS_MAX_TIMERP_PER_SOC     (TimerP_numTimerDevices)
#define OSAL_FREERTOS_MAX_SEMAPHOREP_PER_SOC ((uint32_t)256U)
#define OSAL_FREERTOS_MAX_HWIP_PER_SOC    ((uint32_t)64U)
#define OSAL_FREERTOS_MAX_TIMERP_PER_SOC  (TimerP_numTimerDevices)
#define OSAL_FREERTOS_MAX_TASKP_PER_SOC   ((uint32_t)128U)
#define OSAL_FREERTOS_MAX_MUTEXP_PER_SOC  ((uint32_t)32U)

#ifdef __cplusplus
}
#endif
#endif /* _OSAL_SOC_AM62X_H_ */
