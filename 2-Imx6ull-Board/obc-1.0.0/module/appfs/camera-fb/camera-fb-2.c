// camera-fb-v4l2-loop.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include <linux/fb.h>
#include <errno.h>
#include <signal.h>

/* 分辨率固定为 640×480 YUYV */
#define SRC_WIDTH  640
#define SRC_HEIGHT 480

#define CLIP(x) ((x) > 255 ? 255 : ((x) < 0 ? 0 : (x)))

void yuv422_to_rgb888(unsigned char *yuv, unsigned char *rgb, int width, int height)
{
    for (int i = 0; i < width * height / 2; ++i) {
        unsigned char y0 = yuv[4*i + 0];
        unsigned char u  = yuv[4*i + 1];
        unsigned char y1 = yuv[4*i + 2];
        unsigned char v  = yuv[4*i + 3];

        int uu = u - 128;
        int vv = v - 128;

        int r0 = (298 * (y0 - 16) + 409 * vv + 128) >> 8;
        int g0 = (298 * (y0 - 16) - 100 * uu - 208 * vv + 128) >> 8;
        int b0 = (298 * (y0 - 16) + 516 * uu + 128) >> 8;

        int r1 = (298 * (y1 - 16) + 409 * vv + 128) >> 8;
        int g1 = (298 * (y1 - 16) - 100 * uu - 208 * vv + 128) >> 8;
        int b1 = (298 * (y1 - 16) + 516 * uu + 128) >> 8;

        rgb[(2*i+0)*3 + 0] = CLIP(r0);
        rgb[(2*i+0)*3 + 1] = CLIP(g0);
        rgb[(2*i+0)*3 + 2] = CLIP(b0);

        rgb[(2*i+1)*3 + 0] = CLIP(r1);
        rgb[(2*i+1)*3 + 1] = CLIP(g1);
        rgb[(2*i+1)*3 + 2] = CLIP(b1);
    }
}

static int xioctl(int fd, int request, void *arg)
{
    int r;
    do {
        r = ioctl(fd, request, arg);
    } while (r == -1 && errno == EINTR);
    return r;
}

static int camera_init(int fd)
{
    struct v4l2_format fmt = {0};
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = SRC_WIDTH;
    fmt.fmt.pix.height      = SRC_HEIGHT;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    fmt.fmt.pix.field       = V4L2_FIELD_ANY;
    if (xioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
        perror("VIDIOC_S_FMT");
        return -1;
    }

    if (xioctl(fd, VIDIOC_G_FMT, &fmt) == -1) {
        perror("VIDIOC_G_FMT");
        return -1;
    }
    if (fmt.fmt.pix.width != SRC_WIDTH || fmt.fmt.pix.height != SRC_HEIGHT) {
        fprintf(stderr, "Warn: driver gave %dx%d instead of %dx%d\n",
                fmt.fmt.pix.width, fmt.fmt.pix.height,
                SRC_WIDTH, SRC_HEIGHT);
    }
    return 0;
}

#define BUFFER_COUNT 2  // 使用双缓冲减少丢帧

typedef struct {
    void *start;
    size_t length;
} buffer_t;

static int camera_start_streaming(int fd, buffer_t *buffers)
{
    struct v4l2_requestbuffers req = {0};
    req.count  = BUFFER_COUNT;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (xioctl(fd, VIDIOC_REQBUFS, &req) == -1) {
        perror("VIDIOC_REQBUFS");
        return -1;
    }

    for (unsigned int i = 0; i < req.count; i++) {
        struct v4l2_buffer buf = {0};
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;
        if (xioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
            perror("VIDIOC_QUERYBUF");
            return -1;
        }

        buffers[i].length = buf.length;
        buffers[i].start = mmap(NULL, buf.length,
                                PROT_READ | PROT_WRITE,
                                MAP_SHARED, fd, buf.m.offset);
        if (buffers[i].start == MAP_FAILED) {
            perror("mmap");
            return -1;
        }

        if (xioctl(fd, VIDIOC_QBUF, &buf) == -1) {
            perror("VIDIOC_QBUF");
            return -1;
        }
    }

    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (xioctl(fd, VIDIOC_STREAMON, &type) == -1) {
        perror("VIDIOC_STREAMON");
        return -1;
    }
    return 0;
}

static void camera_stop_streaming(int fd, buffer_t *buffers)
{
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    xioctl(fd, VIDIOC_STREAMOFF, &type);

    for (int i = 0; i < BUFFER_COUNT; i++) {
        if (buffers[i].start)
            munmap(buffers[i].start, buffers[i].length);
    }
}

static unsigned char *camera_dequeue_frame(int fd, buffer_t *buffers, size_t *len)
{
    struct v4l2_buffer buf = {0};
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (xioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
        perror("VIDIOC_DQBUF");
        return NULL;
    }

    *len = buf.length;
    unsigned char *data = (unsigned char *)buffers[buf.index].start;

    // 立即重新入队，保持流水线
    if (xioctl(fd, VIDIOC_QBUF, &buf) == -1) {
        perror("VIDIOC_QBUF (requeue)");
    }

    return data;
}

static int fb_display(unsigned char *rgb)
{
    static int first = 1;
    static int fbfd = -1;
    static struct fb_var_screeninfo vinfo;
    static unsigned char *fbp = MAP_FAILED;
    static size_t screensize = 0;

    if (first) {
        fbfd = open("/dev/fb0", O_RDWR);
        if (fbfd == -1) {
            perror("open /dev/fb0");
            return -1;
        }
        if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
            perror("FBIOGET_VSCREENINFO");
            close(fbfd);
            return -1;
        }
        screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;
        fbp = mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
        if (fbp == MAP_FAILED) {
            perror("mmap fb");
            close(fbfd);
            return -1;
        }
        first = 0;
    }

    int dst_w = vinfo.xres, dst_h = vinfo.yres;
    int offset_x = (dst_w - SRC_WIDTH)  / 2;
    int offset_y = (dst_h - SRC_HEIGHT) / 2;
    int bpp = vinfo.bits_per_pixel / 8;

    for (int y = 0; y < SRC_HEIGHT; y++) {
        unsigned char *src_row = rgb + y * SRC_WIDTH * 3;
        unsigned char *dst_row = fbp + ((offset_y + y) * dst_w + offset_x) * bpp;
        for (int x = 0; x < SRC_WIDTH; x++) {
            dst_row[x*bpp+0] = src_row[x*3+2]; // B
            dst_row[x*bpp+1] = src_row[x*3+1]; // G
            dst_row[x*bpp+2] = src_row[x*3+0]; // R
            if (bpp == 4) dst_row[x*bpp+3] = 0;
        }
    }

    return 0;
}

static volatile sig_atomic_t keep_running = 1;

void signal_handler(int sig)
{
    (void)sig;
    keep_running = 0;
}

int main(void)
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    int fd = open("/dev/video0", O_RDWR | O_NONBLOCK);
    if (fd == -1) {
        perror("open /dev/video0");
        return 1;
    }

    if (camera_init(fd) < 0) {
        close(fd);
        return 1;
    }

    buffer_t buffers[BUFFER_COUNT];
    if (camera_start_streaming(fd, buffers) < 0) {
        close(fd);
        return 1;
    }

    unsigned char *rgb = malloc(SRC_WIDTH * SRC_HEIGHT * 3);
    if (!rgb) {
        perror("malloc rgb");
        camera_stop_streaming(fd, buffers);
        close(fd);
        return 1;
    }

    printf("Starting capture loop. Press Ctrl+C to exit.\n");

    while (keep_running) {
        size_t yuv_len;
        unsigned char *yuv = camera_dequeue_frame(fd, buffers, &yuv_len);
        if (!yuv) {
            usleep(10000); // 避免忙等
            continue;
        }

        yuv422_to_rgb888(yuv, rgb, SRC_WIDTH, SRC_HEIGHT);
        fb_display(rgb);
    }

    printf("\nStopping...\n");
    free(rgb);
    camera_stop_streaming(fd, buffers);
    close(fd);
    return 0;
}