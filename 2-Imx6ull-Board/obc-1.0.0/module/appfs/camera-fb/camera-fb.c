// camera-fb.c （纯文件测试版）
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>

// YUV422 (YUYV) to RGB888 转换
void yuv422_to_rgb888(unsigned char *yuv, unsigned char *rgb, int width, int height) {
    for (int i = 0; i < width * height / 2; ++i) {
        // Y0 U Y1 V
        unsigned char y0 = yuv[2 * i + 0];
        unsigned char u  = yuv[2 * i + 1];
        unsigned char y1 = yuv[2 * i + 2];
        unsigned char v  = yuv[2 * i + 3];

        // 转为有符号（减去偏移）
        int uu = (int)u - 128;
        int vv = (int)v - 128;

        // 处理第一个像素 (y0)
        int r0 = (298 * (y0 - 16) + 409 * vv + 128) >> 8;
        int g0 = (298 * (y0 - 16) - 100 * uu - 208 * vv + 128) >> 8;
        int b0 = (298 * (y0 - 16) + 516 * uu + 128) >> 8;

        // 处理第二个 pixel (y1)
        int r1 = (298 * (y1 - 16) + 409 * vv + 128) >> 8;
        int g1 = (298 * (y1 - 16) - 100 * uu - 208 * vv + 128) >> 8;
        int b1 = (298 * (y1 - 16) + 516 * uu + 128) >> 8;

        // 钳位到 [0, 255]
        #define CLIP(x) ((x) > 255 ? 255 : ((x) < 0 ? 0 : (x)))
        rgb[(2*i+0)*3 + 0] = CLIP(r0);   // R
        rgb[(2*i+0)*3 + 1] = CLIP(g0);   // G
        rgb[(2*i+0)*3 + 2] = CLIP(b0);   // B

        rgb[(2*i+1)*3 + 0] = CLIP(r1);   // R
        rgb[(2*i+1)*3 + 1] = CLIP(g1);   // G
        rgb[(2*i+1)*3 + 2] = CLIP(b1);   // B
    }
}

int main() {
    const char *yuv_filename = "yuv_frame.raw";
    int src_width = 640;
    int src_height = 480;

    // === 1. 读取 YUV 文件 ===
    FILE *fp = fopen(yuv_filename, "rb");
    if (!fp) {
        perror("Cannot open yuv_frame.raw");
        return -1;
    }

    // YUV422: 每像素 2 字节 (YUYV)，总大小 = width * height * 2
    size_t yuv_size = src_width * src_height * 2;
    unsigned char *yuv_data = malloc(yuv_size);
    if (fread(yuv_data, 1, yuv_size, fp) != yuv_size) {
        fprintf(stderr, "Error: failed to read full YUV data\n");
        fclose(fp);
        free(yuv_data);
        return -1;
    }
    fclose(fp);

    // === 2. 转 RGB888 ===
    unsigned char *rgb_data = malloc(src_width * src_height * 3);
    yuv422_to_rgb888(yuv_data, rgb_data, src_width, src_height);

    // === 3. 打开 Framebuffer ===
    int fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        perror("Cannot open /dev/fb0");
        free(yuv_data);
        free(rgb_data);
        return -1;
    }

    struct fb_var_screeninfo vinfo;
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        perror("FBIOGET_VSCREENINFO failed");
        close(fbfd);
        free(yuv_data);
        free(rgb_data);
        return -1;
    }

    printf("Framebuffer resolution: %d x %d, bpp=%d\n",
           vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    if (vinfo.bits_per_pixel != 24 && vinfo.bits_per_pixel != 32) {
        fprintf(stderr, "Error: framebuffer must be 24bpp or 32bpp RGB\n");
        close(fbfd);
        free(yuv_data);
        free(rgb_data);
        return -1;
    }

    size_t screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
    unsigned char *fbp = mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (fbp == MAP_FAILED) {
        perror("mmap framebuffer failed");
        close(fbfd);
        free(yuv_data);
        free(rgb_data);
        return -1;
    }

    // === 4. 将图像居中写入 framebuffer ===
    int dst_width = vinfo.xres;   // 应该是 800
    int dst_height = vinfo.yres;  // 应该是 480

    int offset_x = (dst_width - src_width) / 2;   // (800 - 640)/2 = 80
    int offset_y = (dst_height - src_height) / 2; // (480 - 480)/2 = 0

    int bytes_per_pixel = vinfo.bits_per_pixel / 8; // 3 或 4
#if 0
    for (int y = 0; y < src_height; y++) {
        for (int x = 0; x < src_width; x++) {
            int src_idx = (y * src_width + x) * 3;      // RGB888: R,G,B
            int dst_x = offset_x + x;
            int dst_y = offset_y + y;
            int dst_idx = (dst_y * dst_width + dst_x) * bytes_per_pixel;

            // 注意：framebuffer 可能是 BGR 或 RGB，但大多数嵌入式 LCD 是 RGB888
            // 如果颜色不对，可尝试交换 R/B
            fbp[dst_idx + 0] = rgb_data[src_idx + 2];   // B
            fbp[dst_idx + 1] = rgb_data[src_idx + 1];   // G
            fbp[dst_idx + 2] = rgb_data[src_idx + 0];   // R
            if (bytes_per_pixel == 4) {
                fbp[dst_idx + 3] = 0; // Alpha (if any)
            }
        }
    }
#else
    for (int y = 0; y < src_height; y++) {
        unsigned char *src_row = rgb_data + y * src_width * 3;
        unsigned char *dst_row = fbp + ((offset_y + y) * dst_width + offset_x) * bytes_per_pixel;
        for (int x = 0; x < src_width; x++) {
            dst_row[x*3+0] = src_row[x*3+2]; // B
            dst_row[x*3+1] = src_row[x*3+1]; // G
            dst_row[x*3+2] = src_row[x*3+0]; // R
            if (bytes_per_pixel == 4) dst_row[x*3+3] = 0;
        }
    }
#endif
    // === 5. 清理资源 ===
    munmap(fbp, screensize);
    close(fbfd);
    free(yuv_data);
    free(rgb_data);

    printf("Image displayed successfully!\n");
    return 0;
}