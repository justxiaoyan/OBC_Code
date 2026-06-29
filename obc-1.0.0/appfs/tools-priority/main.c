/*
 * priority_dump - Display thread priority and CPU usage information
 * 显示线程优先级和CPU使用情况
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_LINE 1024
#define MAX_COMM 256

/* 线程信息结构体 */
typedef struct {
    int pid;                    /* 线程ID */
    char comm[MAX_COMM];        /* 线程名称 */
    int cpu;                    /* 当前运行的CPU */
    int priority;               /* 优先级 */
    int nice;                   /* NICE值 */
    int policy;                 /* 调度策略 */
    unsigned long utime;        /* 用户态时间 */
    unsigned long stime;        /* 内核态时间 */
    float cpu_usage;            /* CPU占用百分比 */
} thread_info_t;

/* 调度策略宏定义 (与内核保持一致) */
#ifndef SCHED_NORMAL
#define SCHED_NORMAL	0
#endif
#ifndef SCHED_FIFO
#define SCHED_FIFO		1
#endif
#ifndef SCHED_RR
#define SCHED_RR		2
#endif
#ifndef SCHED_BATCH
#define SCHED_BATCH		3
#endif
/* SCHED_ISO: reserved but not implemented yet */
#ifndef SCHED_IDLE
#define SCHED_IDLE		5
#endif
#ifndef SCHED_DEADLINE
#define SCHED_DEADLINE	6
#endif
#ifndef SCHED_EXT
#define SCHED_EXT		7
#endif

/* 调度策略名称 */
static const char* get_policy_name(int policy)
{
    switch (policy) {
        case SCHED_NORMAL:   return "SCHED_NORMAL";
        case SCHED_FIFO:     return "SCHED_FIFO";
        case SCHED_RR:       return "SCHED_RR";
        case SCHED_BATCH:    return "SCHED_BATCH";
        case SCHED_IDLE:     return "SCHED_IDLE";
        case SCHED_DEADLINE: return "SCHED_DEADLINE";
        case SCHED_EXT:      return "SCHED_EXT";
        default:             return "UNKNOWN";
    }
}

/* 读取线程的stat信息 */
static int read_thread_stat(int pid, thread_info_t *info)
{
    char path[256];
    char line[MAX_LINE];
    FILE *fp;
    char *p;
    int ret = -1;

    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    fp = fopen(path, "r");
    if (!fp) {
        return -1;
    }

    if (fgets(line, sizeof(line), fp)) {
        /* 解析 stat 文件 */
        char comm_buf[MAX_COMM] = {0};

        /* 找到命令名（在括号中） */
        p = strchr(line, '(');
        if (p) {
            char *end = strrchr(p, ')');
            if (end) {
                int len = end - p - 1;
                if (len > 0 && len < MAX_COMM - 1) {
                    strncpy(comm_buf, p + 1, len);
                    comm_buf[len] = '\0';
                }
                p = end + 1;

                /* 解析剩余字段 */
                long priority, nice;
                int cpu;

                /* 跳过状态字段，解析到需要的字段 */
                sscanf(p, " %*c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u "
                          "%lu %lu %*d %*d %ld %ld %*d %*d %*u %*u %*d "
                          "%*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u %*d %d",
                          &info->utime, &info->stime, &priority, &nice, &cpu);

                info->priority = (int)priority;
                info->nice = (int)nice;
                info->cpu = cpu;
                strncpy(info->comm, comm_buf, MAX_COMM - 1);
                info->comm[MAX_COMM - 1] = '\0';

                ret = 0;
            }
        }
    }

    fclose(fp);
    return ret;
}

/* 读取线程的调度策略和优先级 */
static int read_thread_sched(int pid, thread_info_t *info)
{
    struct sched_param param;

    info->policy = sched_getscheduler(pid);
    if (info->policy < 0) {
        return -1;
    }

    if (sched_getparam(pid, &param) == 0) {
        /* 实时优先级 */
        if (info->policy == SCHED_FIFO || info->policy == SCHED_RR) {
            info->priority = param.sched_priority;
        }
    }

    return 0;
}

/* 获取所有线程信息 */
static int get_all_threads(thread_info_t **threads, int *count)
{
    DIR *proc_dir;
    struct dirent *entry;
    thread_info_t *thread_list = NULL;
    int thread_count = 0;
    int thread_capacity = 1024;

    /* 分配初始空间 */
    thread_list = malloc(thread_capacity * sizeof(thread_info_t));
    if (!thread_list) {
        fprintf(stderr, "Failed to allocate memory\n");
        return -1;
    }

    /* 打开 /proc 目录 */
    proc_dir = opendir("/proc");
    if (!proc_dir) {
        fprintf(stderr, "Failed to open /proc: %s\n", strerror(errno));
        free(thread_list);
        return -1;
    }

    /* 遍历所有进程 */
    while ((entry = readdir(proc_dir)) != NULL) {
        int pid;
        char task_path[256];
        DIR *task_dir;
        struct dirent *task_entry;

        /* 跳过非数字目录 */
        if (!isdigit(entry->d_name[0])) {
            continue;
        }

        pid = atoi(entry->d_name);

        /* 打开线程目录 */
        snprintf(task_path, sizeof(task_path), "/proc/%d/task", pid);
        task_dir = opendir(task_path);
        if (!task_dir) {
            continue;
        }

        /* 遍历所有线程 */
        while ((task_entry = readdir(task_dir)) != NULL) {
            int tid;

            if (!isdigit(task_entry->d_name[0])) {
                continue;
            }

            tid = atoi(task_entry->d_name);

            /* 扩展数组 */
            if (thread_count >= thread_capacity) {
                thread_capacity *= 2;
                thread_info_t *new_list = realloc(thread_list,
                                                  thread_capacity * sizeof(thread_info_t));
                if (!new_list) {
                    fprintf(stderr, "Failed to reallocate memory\n");
                    closedir(task_dir);
                    closedir(proc_dir);
                    free(thread_list);
                    return -1;
                }
                thread_list = new_list;
            }

            /* 读取线程信息 */
            thread_info_t *info = &thread_list[thread_count];
            memset(info, 0, sizeof(thread_info_t));
            info->pid = tid;

            if (read_thread_stat(tid, info) == 0 &&
                read_thread_sched(tid, info) == 0) {
                thread_count++;
            }
        }

        closedir(task_dir);
    }

    closedir(proc_dir);

    *threads = thread_list;
    *count = thread_count;

    return 0;
}

/* 计算CPU使用率 */
static void calculate_cpu_usage(thread_info_t *threads, int count)
{
    unsigned long total_time = 0;
    int i;

    /* 计算总时间 */
    for (i = 0; i < count; i++) {
        total_time += threads[i].utime + threads[i].stime;
    }

    /* 计算每个线程的占比 */
    if (total_time > 0) {
        for (i = 0; i < count; i++) {
            unsigned long thread_time = threads[i].utime + threads[i].stime;
            threads[i].cpu_usage = (float)thread_time * 100.0 / total_time;
        }
    }
}

/* 比较函数：按CPU使用率排序 */
static int compare_by_cpu_usage(const void *a, const void *b)
{
    const thread_info_t *ta = (const thread_info_t *)a;
    const thread_info_t *tb = (const thread_info_t *)b;

    if (tb->cpu_usage > ta->cpu_usage) return 1;
    if (tb->cpu_usage < ta->cpu_usage) return -1;
    return 0;
}

/* 打印线程信息 */
static void print_thread_info(thread_info_t *threads, int count, int sort_by_cpu)
{
    int i;

    /* 如果需要，按CPU使用率排序 */
    if (sort_by_cpu) {
        qsort(threads, count, sizeof(thread_info_t), compare_by_cpu_usage);
    }

    /* 打印表头 */
    printf("\n");
    printf("%-8s %-35s %-6s %-8s %-6s %-15s %8s\n",
           "PID", "THREAD_NAME", "CPU", "PRIORITY", "NICE", "POLICY", "CPU%");
    printf("%-8s %-35s %-6s %-8s %-6s %-15s %8s\n",
           "--------", "-----------------------------------", "------",
           "--------", "------", "---------------", "--------");

    /* 打印每个线程信息 */
    for (i = 0; i < count; i++) {
        thread_info_t *info = &threads[i];

        printf("%-8d %-35s %-6d %-8d %-6d %-15s %7.2f%%\n",
               info->pid,
               info->comm,
               info->cpu,
               info->priority,
               info->nice,
               get_policy_name(info->policy),
               info->cpu_usage);
    }

    printf("\nTotal threads: %d\n", count);
}

/* 使用说明 */
static void print_usage(const char *prog)
{
    printf("Usage: %s [options]\n", prog);
    printf("Options:\n");
    printf("  -s    Sort by CPU usage (descending)\n");
    printf("  -h    Show this help message\n");
    printf("\n");
    printf("Display all threads with priority, CPU, and scheduling information\n");
}

int main(int argc, char *argv[])
{
    thread_info_t *threads = NULL;
    int thread_count = 0;
    int sort_by_cpu = 0;
    int opt;

    /* 解析命令行参数 */
    while ((opt = getopt(argc, argv, "sh")) != -1) {
        switch (opt) {
            case 's':
                sort_by_cpu = 1;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    /* 获取所有线程信息 */
    if (get_all_threads(&threads, &thread_count) < 0) {
        fprintf(stderr, "Failed to get thread information\n");
        return 1;
    }

    /* 计算CPU使用率 */
    calculate_cpu_usage(threads, thread_count);

    /* 打印信息 */
    print_thread_info(threads, thread_count, sort_by_cpu);

    /* 清理 */
    free(threads);

    return 0;
}
