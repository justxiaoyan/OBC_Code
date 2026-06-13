#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))

// 查找 PXP 设备（通常为 /dev/video0 或 video1）
const char *find_pxp_device() {
    // 你可以硬编码，比如 "/dev/video1"
    // 或通过 v4l2-ctl --list-devices 确认
    return "/dev/video1";
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <input.uyvy> <width> <height>\n", argv[0]);
        return 1;
    }

    const char *input_file = argv[1];
    int width = atoi(argv[2]);
    int height = atoi(argv[3]);
    const char *pxp_dev = find_pxp_device();

    int fd = open(pxp_dev, O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        perror("Cannot open PXP device");
        return 1;
    }

    struct v4l2_format fmt;
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;
    enum v4l2_buf_type type;

    // ===== 1. 设置输入格式 (OUTPUT queue: YUV422) =====
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;  // 支持 UYVY 或 YUYV
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("VIDIOC_S_FMT (output)");
        goto fail;
    }

    // ===== 2. 设置输出格式 (CAPTURE queue: RGB888) =====
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24; // RGB888
    fmt.fmt.pix.field = V4L2_FIELD_NONE;
    if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0) {
        perror("VIDIOC_S_FMT (capture)");
        goto fail;
    }

    // ===== 3. 请求 OUTPUT buffers (用于输入 YUV 数据) =====
    CLEAR(req);
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        perror("REQBUFS output");
        goto fail;
    }

    void *yuv_buffer = NULL;
    size_t yuv_size = width * height * 2; // UYVY: 2 bytes per pixel

    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
        perror("QUERYBUF output");
        goto fail;
    }

    yuv_buffer = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if (yuv_buffer == MAP_FAILED) {
        perror("mmap output");
        goto fail;
    }

    // 读取 YUV 文件到 buffer
    FILE *fp = fopen(input_file, "rb");
    if (!fp) {
        perror("Open input file");
        goto fail_mmap;
    }
    if (fread(yuv_buffer, 1, yuv_size, fp) != yuv_size) {
        fprintf(stderr, "Input file too small\n");
        fclose(fp);
        goto fail_mmap;
    }
    fclose(fp);

    // 将 OUTPUT buffer 入队
    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
        perror("QBUF output");
        goto fail_mmap;
    }

    // ===== 4. 请求 CAPTURE buffers (用于输出 RGB 数据) =====
    CLEAR(req);
    req.count = 1;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0) {
        perror("REQBUFS capture");
        goto fail_mmap;
    }

    void *rgb_buffer = NULL;
    size_t rgb_size = width * height * 3; // RGB24: 3 bytes per pixel

    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;
    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0) {
        perror("QUERYBUF capture");
        goto fail_mmap;
    }

    rgb_buffer = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
    if (rgb_buffer == MAP_FAILED) {
        perror("mmap capture");
        goto fail_mmap;
    }

    // 将 CAPTURE buffer 入队（准备接收结果）
    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) {
        perror("QBUF capture");
        goto fail_mmap2;
    }

    // ===== 5. 启动流 =====
    type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        perror("STREAMON output");
        goto fail_mmap2;
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0) {
        perror("STREAMON capture");
        goto fail_stream;
    }

    // ===== 6. 等待处理完成 =====
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    struct timeval tv = { .tv_sec = 2, .tv_usec = 0 };
    if (select(fd + 1, NULL, NULL, &fds, &tv) <= 0) {
        perror("select timeout");
        goto fail_stream;
    }

    // 出队 CAPTURE buffer（获取 RGB 结果）
    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0) {
        perror("DQBUF capture");
        goto fail_stream;
    }

    // 保存 RGB 到文件
    FILE *out_fp = fopen("output.rgb", "wb");
    if (out_fp) {
        fwrite(rgb_buffer, 1, rgb_size, out_fp);
        fclose(out_fp);
        printf("RGB888 output saved to output.rgb (%dx%d)\n", width, height);
    }

    // ===== 7. 清理 =====
    munmap(rgb_buffer, buf.length);
fail_mmap2:
    munmap(yuv_buffer, yuv_size);
fail_mmap:
fail_stream:
    type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
    ioctl(fd, VIDIOC_STREAMOFF, &type);
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ioctl(fd, VIDIOC_STREAMOFF, &type);
fail:
    close(fd);
    return 0;
}