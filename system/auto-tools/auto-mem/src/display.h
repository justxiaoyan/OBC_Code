/*******************************************************************************
 * AM62x Memory Statistics Tool - Display Module
 * 显示模块
 ******************************************************************************/

#ifndef DISPLAY_H
#define DISPLAY_H

#include "mem_types.h"

/*******************************************************************************
 * 函数声明
 ******************************************************************************/

/* 打印表头 */
void print_header(const char *title);

/* 打印表尾 */
void print_footer(void);

/* 矩阵式显示 */
void display_memory_matrix_new(const memory_statistics_t *stats);

/* 显示简洁摘要 */
void display_summary(const memory_statistics_t *stats);

/* 显示进程详细信息 */
void display_process_details(const processes_mem_info_t *processes, int top_n);

/* 显示线程详细信息 */
void display_thread_details(const threads_mem_info_t *threads, int top_n);

#endif /* DISPLAY_H */
