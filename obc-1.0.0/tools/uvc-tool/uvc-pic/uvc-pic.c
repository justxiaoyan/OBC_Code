#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <string.h>
#include <time.h>

int main()
{
    const char *dev = "/dev/video0"; // 摄像头设备
    int fd = open(dev, O_RDWR);      // 注意改为O_RDWR(需要写权限)
    if (fd < 0)
    {
        perror("打开设备失败");
        return 1;
    }

    // 1. 查询基本能力
    struct v4l2_capability cap = {0};
    if (ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
    {
        perror("查询能力失败");
        close(fd);
        return 1;
    }

    printf("设备信息:\n"
           "  驱动: %s\n"
           "  名称: %s\n"
           "  能力: 0x%08X\n",
           cap.driver, cap.card, cap.capabilities);

    // 2. 枚举支持的视频格式
    struct v4l2_fmtdesc fmt = {
        .index = 0,
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE};

    int mjpeg_supported = 0;
    puts("\n支持格式:");
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmt) == 0)
    {
        printf("  [%d] %s (四字码: %.4s)\n",
               fmt.index, fmt.description, (char *)&fmt.pixelformat);

        // 检查是否支持MJPEG
        if (fmt.pixelformat == V4L2_PIX_FMT_MJPEG)
        {
            mjpeg_supported = 1;
        }
        fmt.index++;
    }

    if (!mjpeg_supported)
    {
        fprintf(stderr, "\n错误：设备不支持MJPEG格式！\n");
        close(fd);
        return 1;
    }

    // 3. 设置MJPEG格式和1080p分辨率
    struct v4l2_format fmt_set = {0};
    fmt_set.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt_set.fmt.pix.width = 640;                     // 1080p宽度
    fmt_set.fmt.pix.height = 480;                    // 1080p高度
    fmt_set.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG; // MJPEG格式
    fmt_set.fmt.pix.field = V4L2_FIELD_NONE;

    if (ioctl(fd, VIDIOC_S_FMT, &fmt_set) < 0)
    {
        perror("设置格式失败");
        close(fd);
        return 1;
    }

    // 确认实际设置的分辨率
    printf("\n已设置格式：%dx%d (四字码: %.4s)\n",
           fmt_set.fmt.pix.width, fmt_set.fmt.pix.height,
           (char *)&fmt_set.fmt.pix.pixelformat);

    // 4. 请求缓冲区
    struct v4l2_requestbuffers req = {0};
    req.count = 1; // 只需1个缓冲区
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP; // 内存映射方式

    if (ioctl(fd, VIDIOC_REQBUFS, &req) < 0)
    {
        perror("请求缓冲区失败");
        close(fd);
        return 1;
    }

    // 5. 查询并映射缓冲区
    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = 0;

    if (ioctl(fd, VIDIOC_QUERYBUF, &buf) < 0)
    {
        perror("查询缓冲区失败");
        close(fd);
        return 1;
    }

    // 映射内存
    void *buffer = mmap(NULL, buf.length,
                        PROT_READ | PROT_WRITE, MAP_SHARED,
                        fd, buf.m.offset);
    if (buffer == MAP_FAILED)
    {
        perror("内存映射失败");
        close(fd);
        return 1;
    }

    // 6. 入队缓冲区
    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0)
    {
        perror("缓冲区入队失败");
        munmap(buffer, buf.length);
        close(fd);
        return 1;
    }

    // 7. 启动视频流
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) < 0)
    {
        perror("启动流失败");
        munmap(buffer, buf.length);
        close(fd);
        return 1;
    }

    // 8. 捕获一帧
    printf("\n正在捕获图像...");
    fflush(stdout);

    if (ioctl(fd, VIDIOC_DQBUF, &buf) < 0)
    {
        perror("捕获帧失败");
        ioctl(fd, VIDIOC_STREAMOFF, &type);
        munmap(buffer, buf.length);
        close(fd);
        return 1;
    }
    printf("完成！\n");

    // 9. 生成文件名（带时间戳）
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char filename[256];
    snprintf(filename, sizeof(filename),
             "capture_%04d%02d%02d_%02d%02d%02d.jpg",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);

    // 10. 保存为JPEG文件
    FILE *fp = fopen(filename, "wb");
    if (!fp)
    {
        perror("创建文件失败");
        ioctl(fd, VIDIOC_STREAMOFF, &type);
        munmap(buffer, buf.length);
        close(fd);
        return 1;
    }

    fwrite(buffer, buf.bytesused, 1, fp);
    fclose(fp);
    printf("已保存图像: %s (%d KB)\n", filename, buf.bytesused / 1024);

    // 11. 重新入队缓冲区（可选）
    if (ioctl(fd, VIDIOC_QBUF, &buf) < 0)
    {
        perror("重新入队失败");
    }

    // 12. 停止视频流
    ioctl(fd, VIDIOC_STREAMOFF, &type);

    // 13. 清理资源
    munmap(buffer, buf.length);

    close(fd);

    return 0;
}