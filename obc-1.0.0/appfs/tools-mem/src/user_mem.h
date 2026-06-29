/*******************************************************************************
 * AM62x Memory Statistics Tool - User Memory Collection
 * 用户态内存采集模块
 ******************************************************************************/

#ifndef USER_MEM_H
#define USER_MEM_H

#include "mem_types.h"

/*******************************************************************************
 * 函数声明
 ******************************************************************************/

/* 采集所有进程内存信息 */
int collect_processes_memory(processes_mem_info_t *processes);

/* 采集线程内存信息 (Top N) */
int collect_threads_memory(threads_mem_info_t *threads, int top_n);

/* 对进程按RSS排序 */
void sort_processes_by_rss(processes_mem_info_t *processes);

/* 对线程按RSS排序 */
void sort_threads_by_rss(threads_mem_info_t *threads);

#endif /* USER_MEM_H */
