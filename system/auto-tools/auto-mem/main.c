/*******************************************************************************
 * AM62x Memory Statistics Tool - Main Program
 * 内存统计工具 - 主程序
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include "mem_types.h"
#include "sys_mem.h"
#include "user_mem.h"
#include "display.h"

/*******************************************************************************
 * 打印使用说明
 ******************************************************************************/
static void print_usage(const char *prog) {
    printf("Usage: %s [options]\n", prog);
    printf("Options:\n");
    printf("  -h          显示帮助信息\n");
    printf("  -t <n>      显示Top N个进程/线程 (默认: 10/20)\n");
    printf("  -q          快速模式 (仅显示概要)\n");
    printf("  -v          详细模式 (显示所有信息)\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s              # 标准输出\n", prog);
    printf("  %s -t 20        # 显示Top 20进程\n", prog);
    printf("  %s -q           # 快速查看\n", prog);
    printf("  %s > report.txt # 保存到文件\n", prog);
}

/*******************************************************************************
 * 主函数
 ******************************************************************************/
int main(int argc, char *argv[]) {
    memory_statistics_t stats;
    int opt;
    int top_n_processes = 10;
    int top_n_threads = 20;
    int quick_mode = 0;
    int verbose_mode = 0;

    /* 解析命令行参数 */
    while ((opt = getopt(argc, argv, "ht:qv")) != -1) {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 't':
                top_n_processes = atoi(optarg);
                top_n_threads = atoi(optarg) * 2;
                if (top_n_processes <= 0) {
                    fprintf(stderr, "Invalid top N value: %s\n", optarg);
                    return 1;
                }
                break;
            case 'q':
                quick_mode = 1;
                break;
            case 'v':
                verbose_mode = 1;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    /* 初始化统计数据 */
    memset(&stats, 0, sizeof(memory_statistics_t));

    /* 1. 采集系统内存信息 */
    if (collect_system_memory(&stats.system) != 0) {
        fprintf(stderr, "Failed to collect system memory info\n");
        return 1;
    }

    if (collect_kernel_memory(&stats.kernel, &stats.system) != 0) {
        fprintf(stderr, "Failed to collect kernel memory info\n");
        return 1;
    }

    if (collect_reserved_memory(&stats.reserved) != 0) {
        fprintf(stderr, "Warning: Failed to collect reserved memory info\n");
    }

    /* 2. 采集用户态内存信息 */
    if (collect_processes_memory(&stats.processes) != 0) {
        fprintf(stderr, "Failed to collect processes memory info\n");
        return 1;
    }

    /* 排序进程 */
    sort_processes_by_rss(&stats.processes);

    /* 采集线程信息 */
    if (collect_threads_memory(&stats.threads, top_n_threads) != 0) {
        fprintf(stderr, "Warning: Failed to collect threads memory info\n");
    }

    /* 3. 计算统计信息 */
    calculate_memory_health(&stats.health, &stats.system);

    /* 显示结果 */
    if (quick_mode) {
        /* 快速模式 - 仅显示概要 */
        display_summary(&stats);
    } else if (verbose_mode) {
        /* 详细模式 - 显示所有信息 */
        display_memory_matrix_new(&stats);
        display_process_details(&stats.processes, top_n_processes);
        if (stats.threads.count > 0) {
            display_thread_details(&stats.threads, top_n_threads);
        }
    } else {
        /* 标准模式 - 矩阵视图 */
        display_memory_matrix_new(&stats);
    }

    return 0;
}
