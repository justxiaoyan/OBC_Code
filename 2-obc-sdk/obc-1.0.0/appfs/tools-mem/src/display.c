/*******************************************************************************
 * AM62x Memory Statistics Tool - Matrix Display Implementation
 * 矩阵式显示模块实现
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "display.h"

/* 颜色定义 */
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_CYAN    "\033[36m"

/* 表格宽度 */
#define TABLE_WIDTH 90
#define LINE_WIDTH 90

/*******************************************************************************
 * 绘制水平分隔线
 ******************************************************************************/
static void print_separator(void) {
    for (int i = 0; i < TABLE_WIDTH; i++) {
        printf("=");
    }
    printf("\n");
}

static void print_dash_line(void) {
    for (int i = 0; i < LINE_WIDTH; i++) {
        printf("-");
    }
    printf("\n");
}

/*******************************************************************************
 * 打印表头
 ******************************************************************************/
void print_header(const char *title) {
    time_t now;
    char timebuf[64];

    time(&now);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", localtime(&now));

    printf("\n");
    print_separator();
    printf(COLOR_BOLD COLOR_CYAN);
    printf("  %s\n", title);
    printf("  %s\n", timebuf);
    printf(COLOR_RESET);
    print_separator();
    printf("\n");
}

/*******************************************************************************
 * 打印表尾
 ******************************************************************************/
void print_footer(void) {
    printf("\n");
    print_separator();
}

/*******************************************************************************
 * 显示内存矩阵 (新格式)
 ******************************************************************************/

/* 定义kernel项结构体 */
typedef struct {
    const char *name;
    mem_size_t size;
} kernel_item_t;

void display_memory_matrix_new(const memory_statistics_t *stats) {
    mem_size_t total_mb = stats->system.total / 1024;

    printf("\n");
    print_separator();

    /* Kernel层 - 按大小降序排列 */
    mem_size_t kernel_total_mb = stats->kernel.total / 1024;
    printf("%-70s %-10lu MB\n", "Kernel", kernel_total_mb);
    print_dash_line();

    /* 创建kernel项数组并排序 */
    kernel_item_t kernel_items[4] = {
        {"OS", stats->kernel.slab / 1024},
        {"KernelStack", stats->kernel.kernel_stack / 1024},
        {"PageTables", stats->kernel.page_tables / 1024},
        {"VmallocUsed", stats->kernel.vmalloc_used / 1024}
    };

    /* 冒泡排序（降序） */
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3 - i; j++) {
            if (kernel_items[j].size < kernel_items[j + 1].size) {
                kernel_item_t temp = kernel_items[j];
                kernel_items[j] = kernel_items[j + 1];
                kernel_items[j + 1] = temp;
            }
        }
    }

    /* 显示排序后的kernel项 */
    for (int i = 0; i < 4; i++) {
        if (kernel_items[i].size > 0) {
            printf("%-70s %-10lu MB\n", kernel_items[i].name, kernel_items[i].size);
        }
    }

    print_separator();

    /* Reserved Memory层 - 按大小降序排列 */
    /* 计算reserved内存总大小 */
    mem_size_t reserved_total_mb = 0;
    for (int i = 0; i < stats->reserved.count; i++) {
        reserved_total_mb += stats->reserved.regions[i].size / 1024 / 1024;
    }
    printf("%-70s %-10lu MB\n", "Reserved_memory", reserved_total_mb);
    print_dash_line();

    /* 创建临时数组用于排序 */
    typedef struct {
        char name[MAX_NAME_LEN];
        mem_size_t size;  /* 字节单位 */
    } reserved_item_t;

    reserved_item_t *sorted_reserved = malloc(stats->reserved.count * sizeof(reserved_item_t));
    if (sorted_reserved) {
        /* 复制数据 */
        for (int i = 0; i < stats->reserved.count; i++) {
            strncpy(sorted_reserved[i].name, stats->reserved.regions[i].name, MAX_NAME_LEN - 1);
            sorted_reserved[i].name[MAX_NAME_LEN - 1] = '\0';
            sorted_reserved[i].size = stats->reserved.regions[i].size;  /* 字节 */
        }

        /* 冒泡排序（降序） */
        for (int i = 0; i < stats->reserved.count - 1; i++) {
            for (int j = 0; j < stats->reserved.count - 1 - i; j++) {
                if (sorted_reserved[j].size < sorted_reserved[j + 1].size) {
                    reserved_item_t temp = sorted_reserved[j];
                    sorted_reserved[j] = sorted_reserved[j + 1];
                    sorted_reserved[j + 1] = temp;
                }
            }
        }

        /* 显示所有保留内存区域（已排序），根据大小选择单位 */
        for (int i = 0; i < stats->reserved.count; i++) {
            mem_size_t size_kb = sorted_reserved[i].size / 1024;
            if (size_kb < 1024) {
                /* 小于1MB，用KB显示 */
                printf("%-70s %-10lu KB\n", sorted_reserved[i].name, size_kb);
            } else {
                /* 大于等于1MB，用MB显示 */
                printf("%-70s %-10lu MB\n", sorted_reserved[i].name, size_kb / 1024);
            }
        }

        free(sorted_reserved);
    }

    print_separator();

    /* User层 - 进程已经在collect时排序，直接显示 */
    mem_size_t user_total_mb = stats->processes.total_rss / 1024;
    int proc_count = stats->processes.count < 20 ? stats->processes.count : 20;

    char user_label[128];
    snprintf(user_label, sizeof(user_label), "User (Top %d / %d processes)",
             proc_count, stats->processes.count);
    printf("%-70s %-10lu MB\n", user_label, user_total_mb);
    print_dash_line();

    /* 显示Top 20进程（已经按RSS降序排列） */
    for (int i = 0; i < proc_count; i++) {
        mem_size_t proc_mb = stats->processes.processes[i].vm_rss / 1024;
        printf("%-70s %-10lu MB\n", stats->processes.processes[i].name, proc_mb);
    }

    print_separator();

    /* 总计信息 */
    mem_size_t used_mb = stats->system.used / 1024;
    mem_size_t free_mb = stats->system.free / 1024;
    mem_size_t available_mb = stats->system.available / 1024;

    /* 计算百分比 */
    int used_percent = stats->health.usage_percent;
    int free_percent = (int)((free_mb * 100) / total_mb);
    int available_percent = (int)((available_mb * 100) / total_mb);

    /* 根据总内存大小选择显示单位 */
    if (total_mb >= 1024) {
        printf("%-50s %9lu GB\n", "total:", total_mb / 1024);
    } else {
        printf("%-50s %9lu MB\n", "total:", total_mb);
    }
    printf("%-50s %9lu MB  (%d%%)\n", "used:", used_mb, used_percent);
    printf("%-50s %9lu MB  (%d%%)\n", "free:", free_mb, free_percent);
    printf("%-50s %9lu MB  (%d%%)\n", "available:", available_mb, available_percent);
    print_separator();
}

/*******************************************************************************
 * 显示简洁的统计信息
 ******************************************************************************/
void display_summary(const memory_statistics_t *stats) {
    mem_size_t total_mb = stats->system.total / 1024;
    mem_size_t used_mb = stats->system.used / 1024;
    mem_size_t free_mb = stats->system.free / 1024;
    mem_size_t kernel_mb = stats->kernel.total / 1024;
    mem_size_t user_mb = stats->processes.total_rss / 1024;

    printf("\n" COLOR_BOLD "内存统计摘要:" COLOR_RESET "\n");
    printf("  总内存:      %6lu MB\n", total_mb);
    printf("  已使用:      %6lu MB\n", used_mb);
    printf("  空闲:        %6lu MB\n", free_mb);
    printf("  内核占用:    %6lu MB\n", kernel_mb);
    printf("  用户态占用:  %6lu MB\n", user_mb);
    printf("  进程数量:    %6d\n", stats->processes.count);

    /* 健康状态 */
    const char *status_str;
    const char *status_color;

    if (stats->health.status == MEM_HEALTH_GOOD) {
        status_str = "健康";
        status_color = COLOR_GREEN;
    } else if (stats->health.status == MEM_HEALTH_WARNING) {
        status_str = "警告";
        status_color = COLOR_YELLOW;
    } else {
        status_str = "危险";
        status_color = "\033[31m";
    }

    printf("  健康状态:    %s%s%s\n", status_color, status_str, COLOR_RESET);
}

/*******************************************************************************
 * 显示详细的进程列表
 ******************************************************************************/
void display_process_details(const processes_mem_info_t *processes, int top_n) {
    printf("\n" COLOR_BOLD "Top %d 进程详细信息:" COLOR_RESET "\n\n", top_n);
    printf("%-8s %-20s %10s %10s %10s\n",
           "PID", "进程名", "RSS(MB)", "VSize(MB)", "占比(%)");
    printf("-------- -------------------- ---------- ---------- ----------\n");

    mem_size_t total_rss = processes->total_rss;
    int count = processes->count < top_n ? processes->count : top_n;

    for (int i = 0; i < count; i++) {
        mem_size_t rss_mb = processes->processes[i].vm_rss / 1024;
        mem_size_t vsize_mb = processes->processes[i].vm_size / 1024;
        int percent = (int)((processes->processes[i].vm_rss * 100) / total_rss);

        printf("%-8d %-20s %10lu %10lu %10d\n",
               processes->processes[i].pid,
               processes->processes[i].name,
               rss_mb,
               vsize_mb,
               percent);
    }
}

/*******************************************************************************
 * 显示线程列表
 ******************************************************************************/
void display_thread_details(const threads_mem_info_t *threads, int top_n) {
    printf("\n" COLOR_BOLD "Top %d 线程详细信息:" COLOR_RESET "\n\n", top_n);
    printf("%-8s %-8s %-20s %-20s %10s\n",
           "PID", "TID", "进程名", "线程名", "RSS(MB)");
    printf("-------- -------- -------------------- -------------------- ----------\n");

    int count = threads->count < top_n ? threads->count : top_n;

    for (int i = 0; i < count; i++) {
        mem_size_t rss_mb = threads->threads[i].vm_rss / 1024;

        printf("%-8d %-8d %-20s %-20s %10lu\n",
               threads->threads[i].pid,
               threads->threads[i].tid,
               threads->threads[i].proc_name,
               threads->threads[i].thread_name,
               rss_mb);
    }
}
