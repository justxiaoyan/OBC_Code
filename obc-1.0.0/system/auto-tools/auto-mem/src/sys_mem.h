/*******************************************************************************
 * AM62x Memory Statistics Tool - System Memory Collection
 * 系统内存采集模块
 ******************************************************************************/

#ifndef SYS_MEM_H
#define SYS_MEM_H

#include "mem_types.h"

/*******************************************************************************
 * 函数声明
 ******************************************************************************/

/* 采集系统内存信息 (/proc/meminfo) */
int collect_system_memory(system_mem_info_t *info);

/* 采集内核内存信息 */
int collect_kernel_memory(kernel_mem_info_t *kernel, const system_mem_info_t *sys);

/* 采集保留内存信息 (设备树/proc/iomem) */
int collect_reserved_memory(reserved_mem_info_t *reserved);

/* 计算内存健康状态 */
void calculate_memory_health(memory_health_t *health, const system_mem_info_t *sys);

#endif /* SYS_MEM_H */
