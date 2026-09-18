/*******************************************************************************
 * AM62x Memory Statistics Tool - Data Types
 * 内存统计工具 - 数据结构定义
 ******************************************************************************/

#ifndef MEM_TYPES_H
#define MEM_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>

/* 最大数量限制 */
#define MAX_PROCESSES       1024
#define MAX_THREADS         1024
#define MAX_RESERVED_REGIONS 100
#define MAX_NAME_LEN        256
#define MAX_PATH_LEN        512

/* 内存大小类型 (KB) */
typedef uint64_t mem_size_t;

/*******************************************************************************
 * 系统内存信息
 ******************************************************************************/
typedef struct {
    mem_size_t total;           /* 总内存 */
    mem_size_t free;            /* 空闲内存 */
    mem_size_t available;       /* 可用内存 */
    mem_size_t buffers;         /* 缓冲区 */
    mem_size_t cached;          /* 缓存 */
    mem_size_t slab;            /* Slab */
    mem_size_t used;            /* 已使用 */
    mem_size_t kernel_stack;    /* 内核栈 */
    mem_size_t page_tables;     /* 页表 */
    mem_size_t vmalloc_used;    /* Vmalloc使用量 */
} system_mem_info_t;

/*******************************************************************************
 * 内核内存信息
 ******************************************************************************/
typedef struct {
    mem_size_t slab;            /* Slab缓存 */
    mem_size_t kernel_stack;    /* 内核栈 */
    mem_size_t page_tables;     /* 页表 */
    mem_size_t vmalloc_used;    /* Vmalloc使用 */
    mem_size_t total;           /* 内核总占用 */
} kernel_mem_info_t;

/*******************************************************************************
 * 保留内存信息
 ******************************************************************************/
typedef struct {
    char name[MAX_NAME_LEN];    /* 区域名称 */
    uint64_t start_addr;        /* 起始地址 */
    uint64_t end_addr;          /* 结束地址 */
    mem_size_t size;            /* 大小(字节) */
} reserved_mem_region_t;

typedef struct {
    reserved_mem_region_t regions[MAX_RESERVED_REGIONS];
    int count;                  /* 区域数量 */
} reserved_mem_info_t;

/*******************************************************************************
 * 进程内存信息
 ******************************************************************************/
typedef struct {
    pid_t pid;                  /* 进程ID */
    char name[MAX_NAME_LEN];    /* 进程名 */
    mem_size_t vm_rss;          /* 物理内存 */
    mem_size_t vm_size;         /* 虚拟内存 */
    mem_size_t vm_data;         /* 数据段 */
    mem_size_t vm_stk;          /* 栈 */
    mem_size_t vm_exe;          /* 代码段 */
    mem_size_t vm_lib;          /* 库 */
} process_mem_info_t;

typedef struct {
    process_mem_info_t processes[MAX_PROCESSES];
    int count;                  /* 进程数量 */
    mem_size_t total_rss;       /* RSS总计 */
    mem_size_t total_vsize;     /* VSize总计 */
} processes_mem_info_t;

/*******************************************************************************
 * 线程内存信息
 ******************************************************************************/
typedef struct {
    pid_t pid;                  /* 进程ID */
    pid_t tid;                  /* 线程ID */
    char proc_name[MAX_NAME_LEN]; /* 进程名 */
    char thread_name[MAX_NAME_LEN]; /* 线程名 */
    mem_size_t vm_rss;          /* 内存占用 */
} thread_mem_info_t;

typedef struct {
    thread_mem_info_t threads[MAX_THREADS];
    int count;                  /* 线程数量 */
} threads_mem_info_t;

/*******************************************************************************
 * 内存健康状态
 ******************************************************************************/
typedef enum {
    MEM_HEALTH_GOOD = 0,        /* 健康 */
    MEM_HEALTH_WARNING,         /* 警告 */
    MEM_HEALTH_CRITICAL         /* 危险 */
} mem_health_status_t;

typedef struct {
    mem_health_status_t status; /* 健康状态 */
    int usage_percent;          /* 使用率百分比 */
    int available_percent;      /* 可用内存百分比 */
} memory_health_t;

/*******************************************************************************
 * 完整的内存统计数据
 ******************************************************************************/
typedef struct {
    system_mem_info_t system;       /* 系统内存 */
    kernel_mem_info_t kernel;       /* 内核内存 */
    reserved_mem_info_t reserved;   /* 保留内存 */
    processes_mem_info_t processes; /* 进程信息 */
    threads_mem_info_t threads;     /* 线程信息 */
    memory_health_t health;         /* 健康状态 */
} memory_statistics_t;

#endif /* MEM_TYPES_H */
