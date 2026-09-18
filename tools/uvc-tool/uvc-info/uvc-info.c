
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
 
int main()
{

    const char* dev = "/dev/video0";  // 摄像头设备
    int fd = open(dev, O_RDONLY);     // 打开设备

    // 1. 查询基本能力
    struct v4l2_capability cap = {0};
    ioctl(fd, VIDIOC_QUERYCAP, &cap);
 
    printf("设备信息:\n"
           "  驱动: %s\n"
           "  名称: %s\n"
           "  能力: 0x%08X\n", 
           cap.driver, cap.card, cap.capabilities);

    // 2. 枚举支持的视频格式
    struct v4l2_fmtdesc fmt = {
        .index = 0,
        .type = V4L2_BUF_TYPE_VIDEO_CAPTURE
    };
 
    puts("\n支持格式:");
    while (ioctl(fd, VIDIOC_ENUM_FMT, &fmt) == 0) {
        printf("  [%d] %s (四字码: %.4s)\n", 
               fmt.index, fmt.description, (char*)&fmt.pixelformat);
        fmt.index++;
    }

    close(fd);

    return 0;
}