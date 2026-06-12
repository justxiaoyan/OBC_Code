#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 5240  // 1KB
#define TIME_THRESHOLD_MS 200  // 200ms阈值

// 获取当前时间戳（毫秒）
long long get_current_timestamp_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000LL + ts.tv_nsec / 1000000LL;
}

int main() {
    char buffer[BUFFER_SIZE];

    // 填充1KB的数据（这里用'A'填充，您可以根据需要修改）
    memset(buffer, 'A', BUFFER_SIZE - 1);
    buffer[BUFFER_SIZE - 1] = '\0';  // 确保字符串以null结尾

    while (1) {
        // 获取开始时间戳
        long long start_time = get_current_timestamp_ms();

        // 打印开始时间戳
//        printf("[开始时间戳: %lldms] ", start_time);

        // 使用printf输出1KB信息
        printf("%s", buffer);

        // 获取结束时间戳
        long long end_time = get_current_timestamp_ms();

        // 打印结束时间戳
  //      printf(" [结束时间戳: %lldms]", end_time);

        // 计算printf耗时
        long long duration = end_time - start_time;

        // 如果耗时大于200ms，打印耗时信息
        if (duration > TIME_THRESHOLD_MS) {
            printf(" [error: printf time  %lldms > 200ms]", duration);
        }

        printf("\n");  // 换行

        // 等待100ms
//        usleep(10 * 1000);  // 100ms = 100 * 1000 微秒
    }

    return 0;
}
