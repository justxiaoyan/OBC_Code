/*******************************************************************************
 * AM62x Memory Statistics Tool - User Memory Collection Implementation
 * 用户态内存采集模块实现
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include "user_mem.h"

/*******************************************************************************
 * 辅助函数 - 检查字符串是否全为数字
 ******************************************************************************/
static int is_numeric(const char *str) {
    if (!str || !*str) return 0;
    while (*str) {
        if (!isdigit(*str)) return 0;
        str++;
    }
    return 1;
}

/*******************************************************************************
 * 辅助函数 - 从/proc/[pid]/status读取进程信息
 ******************************************************************************/
static int read_process_status(pid_t pid, process_mem_info_t *proc) {
    char path[256];
    char line[256];
    FILE *fp;

    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    fp = fopen(path, "r");
    if (!fp) return -1;

    proc->pid = pid;
    proc->vm_rss = 0;
    proc->vm_size = 0;
    proc->vm_data = 0;
    proc->vm_stk = 0;
    proc->vm_exe = 0;
    proc->vm_lib = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "Name:", 5) == 0) {
            sscanf(line, "Name: %255s", proc->name);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS: %lu kB", &proc->vm_rss);
        } else if (strncmp(line, "VmSize:", 7) == 0) {
            sscanf(line, "VmSize: %lu kB", &proc->vm_size);
        } else if (strncmp(line, "VmData:", 7) == 0) {
            sscanf(line, "VmData: %lu kB", &proc->vm_data);
        } else if (strncmp(line, "VmStk:", 6) == 0) {
            sscanf(line, "VmStk: %lu kB", &proc->vm_stk);
        } else if (strncmp(line, "VmExe:", 6) == 0) {
            sscanf(line, "VmExe: %lu kB", &proc->vm_exe);
        } else if (strncmp(line, "VmLib:", 6) == 0) {
            sscanf(line, "VmLib: %lu kB", &proc->vm_lib);
        }
    }

    fclose(fp);
    return 0;
}

/*******************************************************************************
 * 采集所有进程内存信息
 ******************************************************************************/
int collect_processes_memory(processes_mem_info_t *processes) {
    DIR *dir;
    struct dirent *entry;

    if (!processes) return -1;
    memset(processes, 0, sizeof(processes_mem_info_t));

    dir = opendir("/proc");
    if (!dir) {
        fprintf(stderr, "Failed to open /proc: %s\n", strerror(errno));
        return -1;
    }

    while ((entry = readdir(dir)) != NULL && processes->count < MAX_PROCESSES) {
        /* 只处理数字命名的目录 (进程ID) */
        if (!is_numeric(entry->d_name)) continue;

        pid_t pid = atoi(entry->d_name);
        process_mem_info_t *proc = &processes->processes[processes->count];

        if (read_process_status(pid, proc) == 0 && proc->vm_rss > 0) {
            processes->total_rss += proc->vm_rss;
            processes->total_vsize += proc->vm_size;
            processes->count++;
        }
    }

    closedir(dir);
    return 0;
}

/*******************************************************************************
 * 辅助函数 - 从/proc/[pid]/task/[tid]/status读取线程信息
 ******************************************************************************/
static int read_thread_status(pid_t pid, pid_t tid, thread_mem_info_t *thread,
                              const char *proc_name) {
    char path[256];
    char line[256];
    FILE *fp;

    snprintf(path, sizeof(path), "/proc/%d/task/%d/status", pid, tid);
    fp = fopen(path, "r");
    if (!fp) return -1;

    thread->pid = pid;
    thread->tid = tid;
    size_t name_len = strlen(proc_name);
    if (name_len >= MAX_NAME_LEN) {
        name_len = MAX_NAME_LEN - 1;
    }
    memcpy(thread->proc_name, proc_name, name_len);
    thread->proc_name[name_len] = '\0';
    thread->vm_rss = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "Name:", 5) == 0) {
            sscanf(line, "Name: %255s", thread->thread_name);
        } else if (strncmp(line, "VmRSS:", 6) == 0) {
            sscanf(line, "VmRSS: %lu kB", &thread->vm_rss);
        }
    }

    fclose(fp);
    return 0;
}

/*******************************************************************************
 * 采集线程内存信息 (Top N)
 ******************************************************************************/
int collect_threads_memory(threads_mem_info_t *threads, int top_n) {
    DIR *proc_dir, *task_dir;
    struct dirent *proc_entry, *task_entry;
    char task_path[256];
    char proc_name[MAX_NAME_LEN];

    if (!threads) return -1;
    memset(threads, 0, sizeof(threads_mem_info_t));

    proc_dir = opendir("/proc");
    if (!proc_dir) {
        fprintf(stderr, "Failed to open /proc: %s\n", strerror(errno));
        return -1;
    }

    /* 遍历所有进程 */
    while ((proc_entry = readdir(proc_dir)) != NULL) {
        if (!is_numeric(proc_entry->d_name)) continue;

        pid_t pid = atoi(proc_entry->d_name);

        /* 获取进程名 */
        char status_path[256];
        snprintf(status_path, sizeof(status_path), "/proc/%d/status", pid);
        FILE *fp = fopen(status_path, "r");
        if (fp) {
            char line[256];
            proc_name[0] = '\0';
            while (fgets(line, sizeof(line), fp)) {
                if (strncmp(line, "Name:", 5) == 0) {
                    sscanf(line, "Name: %255s", proc_name);
                    break;
                }
            }
            fclose(fp);
        }

        /* 打开task目录 */
        snprintf(task_path, sizeof(task_path), "/proc/%d/task", pid);
        task_dir = opendir(task_path);
        if (!task_dir) continue;

        /* 遍历所有线程 */
        while ((task_entry = readdir(task_dir)) != NULL &&
               threads->count < MAX_THREADS) {
            if (!is_numeric(task_entry->d_name)) continue;

            pid_t tid = atoi(task_entry->d_name);
            thread_mem_info_t *thread = &threads->threads[threads->count];

            if (read_thread_status(pid, tid, thread, proc_name) == 0 &&
                thread->vm_rss > 0) {
                threads->count++;
            }
        }
        closedir(task_dir);
    }

    closedir(proc_dir);

    /* 排序并只保留Top N */
    sort_threads_by_rss(threads);
    if (threads->count > top_n) {
        threads->count = top_n;
    }

    return 0;
}

/*******************************************************************************
 * 对进程按RSS排序 (降序)
 ******************************************************************************/
static int compare_process_rss(const void *a, const void *b) {
    const process_mem_info_t *pa = (const process_mem_info_t *)a;
    const process_mem_info_t *pb = (const process_mem_info_t *)b;
    return (pb->vm_rss > pa->vm_rss) - (pb->vm_rss < pa->vm_rss);
}

void sort_processes_by_rss(processes_mem_info_t *processes) {
    if (!processes || processes->count == 0) return;
    qsort(processes->processes, processes->count,
          sizeof(process_mem_info_t), compare_process_rss);
}

/*******************************************************************************
 * 对线程按RSS排序 (降序)
 ******************************************************************************/
static int compare_thread_rss(const void *a, const void *b) {
    const thread_mem_info_t *ta = (const thread_mem_info_t *)a;
    const thread_mem_info_t *tb = (const thread_mem_info_t *)b;
    return (tb->vm_rss > ta->vm_rss) - (tb->vm_rss < ta->vm_rss);
}

void sort_threads_by_rss(threads_mem_info_t *threads) {
    if (!threads || threads->count == 0) return;
    qsort(threads->threads, threads->count,
          sizeof(thread_mem_info_t), compare_thread_rss);
}
