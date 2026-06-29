/*
 * priority_demo - Thread priority demonstration
 * 演示不同调度策略的线程（RR、FIFO、NORMAL）
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/types.h>

/* 获取线程ID（兼容不同glibc版本） */
static pid_t get_thread_id(void)
{
    return syscall(SYS_gettid);
}

/* 线程参数结构 */
typedef struct {
    const char *name;
    int policy;
    int priority;
} thread_param_t;

/* 线程工作函数 */
static void* thread_worker(void *arg)
{
    thread_param_t *param = (thread_param_t *)arg;
    pid_t tid = get_thread_id();
    struct sched_param sched_param;

    /* 设置线程调度策略和优先级 */
    sched_param.sched_priority = param->priority;
    if (pthread_setschedparam(pthread_self(), param->policy, &sched_param) != 0) {
        fprintf(stderr, "[%s] Failed to set scheduling parameters: %s\n",
                param->name, strerror(errno));
        fprintf(stderr, "       Note: Real-time scheduling requires root privileges\n");
        return NULL;
    }

    /* 打印线程创建成功信息 */
    printf("[%s] Created successfully - TID: %d\n", param->name, tid);

    /* 执行一些工作，但不要100%占用CPU */
    unsigned long counter = 0;
    while (1) {
        counter++;

        /* 所有线程都适当休眠，展示调度策略而不是烧CPU */
        usleep(10000);  /* 10ms - 演示用，避免CPU满载 */

        /* 如果需要测试CPU密集型场景，可以注释掉上面的usleep */
    }

    return NULL;
}

/* 打印使用说明 */
static void print_usage(const char *prog)
{
    printf("Usage: %s [options]\n", prog);
    printf("Options:\n");
    printf("  -h    Show this help message\n");
    printf("\n");
    printf("Create three threads with different scheduling policies:\n");
    printf("  - RR (Round Robin) with priority 50\n");
    printf("  - FIFO (First In First Out) with priority 30\n");
    printf("  - NORMAL (CFS) with priority 0\n");
    printf("\n");
    printf("Note: Real-time scheduling (RR/FIFO) requires root privileges.\n");
    printf("      Run with 'sudo' if you see permission errors.\n");
}

int main(int argc, char *argv[])
{
    pthread_t threads[3];
    pthread_attr_t attr[3];
    thread_param_t params[3];
    int i;

    /* 解析命令行参数 */
    if (argc > 1 && strcmp(argv[1], "-h") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    printf("====================================\n");
    printf("Thread Priority Demonstration\n");
    printf("====================================\n");

    /* 检查是否有root权限 */
    if (geteuid() != 0) {
        printf("Warning: Not running as root. Real-time threads may fail.\n");
    }

    /* 配置线程1: RR调度 */
    params[0].name = "RR-Thread";
    params[0].policy = SCHED_RR;
    params[0].priority = 50;

    /* 配置线程2: FIFO调度 */
    params[1].name = "FIFO-Thread";
    params[1].policy = SCHED_FIFO;
    params[1].priority = 30;

    /* 配置线程3: NORMAL调度 */
    params[2].name = "NORMAL-Thread";
    params[2].policy = SCHED_NORMAL;
    params[2].priority = 0;

    /* 创建线程 */
    for (i = 0; i < 3; i++) {
        /* 初始化线程属性 */
        if (pthread_attr_init(&attr[i]) != 0) {
            fprintf(stderr, "Failed to initialize thread attributes\n");
            return 1;
        }

        /* 设置继承调度属性 */
        pthread_attr_setinheritsched(&attr[i], PTHREAD_EXPLICIT_SCHED);

        /* 设置调度策略 */
        pthread_attr_setschedpolicy(&attr[i], params[i].policy);

        /* 设置优先级 */
        struct sched_param sched_param;
        sched_param.sched_priority = params[i].priority;
        pthread_attr_setschedparam(&attr[i], &sched_param);

        /* 创建线程 */
        if (pthread_create(&threads[i], &attr[i], thread_worker, &params[i]) != 0) {
            fprintf(stderr, "Failed to create thread %d: %s\n", i, strerror(errno));
            return 1;
        }

        /* 清理线程属性 */
        pthread_attr_destroy(&attr[i]);

        /* 短暂延迟，让线程初始化完成 */
        usleep(100000);  /* 100ms */
    }

    printf("====================================\n");
    printf("\nUse 'priority_dump' to view thread details.\n");
    printf("Press Ctrl+C to exit.\n\n");

    /* 主线程进入休眠，等待信号（避免占用CPU） */
    while (1) {
        sleep(3600);  /* 休眠1小时 */
    }

    return 0;
}
