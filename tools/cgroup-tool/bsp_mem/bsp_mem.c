#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define BLOCK_SIZE 4096

int main(int argc, char *argv[])
{
    long input_size = 0;
    
    if (argc != 2)
    {
        printf("to few argument\n");
        printf("example: bsp_mem 20971520(20M)\n");
        return -1;
    }

    input_size = atoll(argv[1]);

    if (input_size <= 0)
    {
        printf("input size error\n");
        return -1;
    }

    // 检查输入是否满足4K对齐
    if (input_size % BLOCK_SIZE != 0) {
        printf("Warning:input_size not 4K aligned, auto-adjusted\n");
        long aligned_size = ((input_size / BLOCK_SIZE) + 1) * BLOCK_SIZE;
        printf("input size: %ld, after adjusting: %d\n", input_size, aligned_size);
        // 重新分配内存
        char *memory = (char *)malloc(aligned_size);
        if (memory == NULL) {
            printf("malloc error\n");
            return -1;
        }
        printf("malloc %d byte memory\n", input_size);
        // 循环填充随机数
        srand(time(NULL));
        while (1) {
            // 填充每个块
            for (int i = 0; i < aligned_size; i += BLOCK_SIZE) {
                // 生成随机数并填充块
                for (int j = 0; j < BLOCK_SIZE; j++) {
                    memory[i + j] = rand() % 256;
                }
            }
            sleep(10); // 每10s循环一次
        }
    } else {
        // 如果已经是4K对齐
        char *memory = (char *)malloc(input_size);
        if (memory == NULL) {
            printf("内存分配失败\n");
            return -1;
        }
        printf("malloc %d byte memory\n", input_size);
        // 循环填充随机数
        srand(time(NULL));
        while (1) {
            // 填充每个块
            for (int i = 0; i < input_size; i += BLOCK_SIZE) {
                // 生成随机数并填充块
                for (int j = 0; j < BLOCK_SIZE; j++) {
                    memory[i + j] = rand() % 256;
                }
            }
            sleep(10); // 每10s循环一次
        }
    }
    return 0;
}
