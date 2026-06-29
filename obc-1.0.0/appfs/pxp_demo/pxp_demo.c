#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

// --- 从你提供的头文件中提取关键定义 ---
#define fourcc(a, b, c, d) ((uint32_t)(a) | ((uint32_t)(b) << 8) | \
                            ((uint32_t)(c) << 16) | ((uint32_t)(d) << 24))

#define PXP_PIX_FMT_YUYV    fourcc('Y', 'U', 'Y', 'V')
#define PXP_PIX_FMT_RGB24   fourcc('R', 'G', 'B', '3')  // RGB888

// 假设驱动支持这个 ioctl（多数 NXP BSP 支持）
#define PXP_IOC_MAGIC 'P'
#define PXP_IOC_CONFIG _IOW(PXP_IOC_MAGIC, 10, struct pxp_config_data)

struct pxp_layer_param {
    int width;
    int height;
    uint32_t format;
    unsigned long phys_addr;  // 注意：有些驱动用 phys，有些用 virt
    int stride;
};

struct pxp_config_data {
    struct pxp_layer_param input;
    struct pxp_layer_param output;
    unsigned int flags;
};

// --- 主程序 ---
int main(void)
{
    const char *input_file = "yuv_frame.raw";
    const char *output_file = "rgb888_output.raw";
    int width = 640, height = 480;
    size_t yuv_size = width * height * 2;
    size_t rgb_size = width * height * 3;

    int fd = open("/dev/pxp_device", O_RDWR);
    if (fd < 0) {
        perror("open /dev/pxp_device");
        return -1;
    }

    // mmap 两块连续内存（驱动通常要求物理连续，mmap 会分配）
    void *in_buf = mmap(NULL, yuv_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (in_buf == MAP_FAILED) {
        perror("mmap in");
        close(fd);
        return -1;
    }

    void *out_buf = mmap(NULL, rgb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, yuv_size);
    if (out_buf == MAP_FAILED) {
        perror("mmap out");
        munmap(in_buf, yuv_size);
        close(fd);
        return -1;
    }

    // 读入 YUV
    FILE *fin = fopen(input_file, "rb");
    if (!fin || fread(in_buf, 1, yuv_size, fin) != yuv_size) {
        perror("read yuv");
        goto fail;
    }
    fclose(fin);

    // 准备配置
    struct pxp_config_data cfg = {0};
    cfg.input.width = width;
    cfg.input.height = height;
    cfg.input.format = PXP_PIX_FMT_YUYV;
    cfg.input.stride = width * 2;
    cfg.input.phys_addr = 0; // 驱动从 mmap offset 推断物理地址

    cfg.output.width = width;
    cfg.output.height = height;
    cfg.output.format = PXP_PIX_FMT_RGB24; // RGB888
    cfg.output.stride = width * 3;
    cfg.output.phys_addr = 0;

    // 执行转换
    if (ioctl(fd, PXP_IOC_CONFIG, &cfg) < 0) {
        perror("PXP_IOC_CONFIG failed");
        goto fail;
    }

    // 写出 RGB
    FILE *fout = fopen(output_file, "wb");
    if (!fout || fwrite(out_buf, 1, rgb_size, fout) != rgb_size) {
        perror("write rgb");
    } else {
        printf("Success: %s -> %s\n", input_file, output_file);
    }
    if (fout) fclose(fout);

fail:
    munmap(out_buf, rgb_size);
    munmap(in_buf, yuv_size);
    close(fd);
    return 0;
}