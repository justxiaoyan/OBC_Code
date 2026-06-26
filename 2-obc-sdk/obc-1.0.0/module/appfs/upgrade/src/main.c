/*
 * EMS Upgrade Tool - Main Entry
 * AM62x User-space Upgrade Utility
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include "upgrade.h"

/* 进度回调函数 */
static void progress_callback(int progress, const char *msg)
{
    static int last_progress = -1;

    if (progress != last_progress) {
        printf("\r[%3d%%] %s", progress, msg);
        fflush(stdout);
        last_progress = progress;

        if (progress == 100) {
            printf("\n");
        }
    }
}

/* 打印使用帮助 */
static void print_usage(const char *prog)
{
    printf("Usage: %s -<type> <image_file> [options]\n", prog);
    printf("\n");
    printf("Upgrade types:\n");
    printf("  -loader          Upgrade bootloader\n");
    printf("  -uboot           Upgrade U-Boot\n");
    printf("  -fdt             Upgrade device tree\n");
    printf("  -atf             Upgrade ARM Trusted Firmware\n");
    printf("  -teeos           Upgrade TEE OS\n");
    printf("  -kernel          Upgrade kernel\n");
    printf("  -appfs           Upgrade application filesystem\n");
    printf("  -rootfs          Upgrade root filesystem\n");
    printf("\n");
    printf("Options:\n");
    printf("  -f, --format <type>   Image format (raw/ext4/ext4-sparse/squashfs/auto)\n");
    printf("  -F, --force           Force upgrade even if size exceeds partition\n");
    printf("  -v, --verify          Verify image before upgrade\n");
    printf("  -h, --help            Show this help message\n");
    printf("  -V, --version         Show version information\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s -k zImage              # Upgrade kernel (short option)\n", prog);
    printf("  %s -d am625-sk.dtb        # Upgrade device tree (short option)\n", prog);
    printf("  %s -l loader.bin          # Upgrade bootloader (short option)\n", prog);
    printf("  %s -kernel zImage         # Upgrade kernel (long option)\n", prog);
    printf("  %s -r rootfs.ext4 -f ext4 # Upgrade rootfs with ext4 format\n", prog);
    printf("\n");
}

/* 打印版本信息 */
static void print_version(void)
{
    printf("EMS Upgrade Tool v%s\n", UPGRADE_VERSION);
    printf("AM62x User-space Upgrade Utility\n");
}

int main(int argc, char *argv[])
{
    upgrade_ctx_t ctx;
    int ret;
    int opt;
    bool type_set = false;

    /* 初始化上下文 */
    memset(&ctx, 0, sizeof(ctx));
    ctx.format = IMAGE_FORMAT_AUTO;
    ctx.verify = false;
    ctx.force = false;

    static struct option long_options[] = {
        {"format",  required_argument, 0, 'f'},
        {"force",   no_argument,       0, 'F'},
        {"verify",  no_argument,       0, 'v'},
        {"help",    no_argument,       0, 'h'},
        {"version", no_argument,       0, 'V'},
        /* 升级类型短选项 */
        {"loader",  required_argument, 0, 'l'},
        {"uboot",   required_argument, 0, 'u'},
        {"kernel",  required_argument, 0, 'k'},
        {"teeos",   required_argument, 0, 't'},
        {"dtb",     required_argument, 0, 'd'},  /* fdt -> dtb */
        {"rootfs",  required_argument, 0, 'r'},
        {0, 0, 0, 0}
    };

    /* 解析命令行参数 */
    while (1) {
        int option_index = 0;
        opt = getopt_long(argc, argv, "l:u:k:t:d:r:f:FvhV", long_options, &option_index);

        if (opt == -1) {
            break;
        }

        switch (opt) {
        /* 升级类型 */
        case 'l':
            ctx.type = UPGRADE_TYPE_LOADER;
            strncpy(ctx.image_path, optarg, sizeof(ctx.image_path) - 1);
            type_set = true;
            break;

        case 'u':
            ctx.type = UPGRADE_TYPE_UBOOT;
            strncpy(ctx.image_path, optarg, sizeof(ctx.image_path) - 1);
            type_set = true;
            break;

        case 'k':
            ctx.type = UPGRADE_TYPE_KERNEL;
            strncpy(ctx.image_path, optarg, sizeof(ctx.image_path) - 1);
            type_set = true;
            break;

        case 't':
            ctx.type = UPGRADE_TYPE_TEEOS;
            strncpy(ctx.image_path, optarg, sizeof(ctx.image_path) - 1);
            type_set = true;
            break;

        case 'd':
            ctx.type = UPGRADE_TYPE_FDT;
            strncpy(ctx.image_path, optarg, sizeof(ctx.image_path) - 1);
            type_set = true;
            break;

        case 'r':
            ctx.type = UPGRADE_TYPE_ROOTFS;
            strncpy(ctx.image_path, optarg, sizeof(ctx.image_path) - 1);
            type_set = true;
            break;

        /* 标准选项 */
        case 'f':
            if (strcmp(optarg, "raw") == 0) {
                ctx.format = IMAGE_FORMAT_RAW;
            } else if (strcmp(optarg, "ext4") == 0) {
                ctx.format = IMAGE_FORMAT_EXT4;
            } else if (strcmp(optarg, "ext4-sparse") == 0) {
                ctx.format = IMAGE_FORMAT_EXT4_SPARSE;
            } else if (strcmp(optarg, "squashfs") == 0) {
                ctx.format = IMAGE_FORMAT_SQUASHFS;
            } else if (strcmp(optarg, "auto") == 0) {
                ctx.format = IMAGE_FORMAT_AUTO;
            } else {
                fprintf(stderr, "Unknown format: %s\n", optarg);
                return 1;
            }
            break;

        case 'F':
            ctx.force = true;
            break;

        case 'v':
            ctx.verify = true;
            break;

        case 'h':
            print_usage(argv[0]);
            return 0;

        case 'V':
            print_version();
            return 0;

        default:
            print_usage(argv[0]);
            return 1;
        }
    }


    /* 检查必需参数 */
    if (!type_set) {
        fprintf(stderr, "Error: Upgrade type not specified\n\n");
        print_usage(argv[0]);
        return 1;
    }

    if (strlen(ctx.image_path) == 0) {
        fprintf(stderr, "Error: Image file not specified\n\n");
        print_usage(argv[0]);
        return 1;
    }

    /* 检查是否以 root 权限运行 */
    if (getuid() != 0) {
        fprintf(stderr, "Warning: This tool should be run as root\n");
    }

    /* 初始化升级 */
    ret = upgrade_init(&ctx);
    if (ret < 0) {
        fprintf(stderr, "Upgrade initialization failed: %d\n", ret);
        return 1;
    }

    /* 执行升级 */
    ret = upgrade_execute(&ctx, progress_callback);
    if (ret < 0) {
        fprintf(stderr, "\nUpgrade failed: %d\n", ret);
        upgrade_cleanup(&ctx);
        return 1;
    }

    /* 清理 */
    upgrade_cleanup(&ctx);

    return 0;
}
