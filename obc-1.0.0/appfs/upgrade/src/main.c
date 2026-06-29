/*
 * EMS Upgrade Tool - Main Entry
 * AM62x User-space Upgrade Utility
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <libgen.h>
#include "upgrade.h"
#include "mkkimg_upgrade.h"
#include "board.h"

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
    printf("Usage:\n");
    printf("  %s factory.bin              Full package upgrade (mkkimg format)\n", prog);
    printf("  %s -<type> <image_file>    Single component upgrade\n", prog);
    printf("\n");
    printf("Full Package Upgrade:\n");
    printf("  %s factory.bin              Upgrade all components from mkkimg package\n", prog);
    printf("                               - Verifies package CRC16\n");
    printf("                               - Verifies each file CRC32\n");
    printf("                               - Verifies signatures\n");
    printf("                               - Upgrades all components\n");
    printf("\n");
    printf("Single Component Upgrade Types:\n");
    printf("  -loader          Upgrade bootloader\n");
    printf("  -uboot           Upgrade U-Boot\n");
    printf("  -fdt             Upgrade device tree\n");
    printf("  -atf             Upgrade ATF\n");
    printf("  -teeos           Upgrade TEE OS\n");
    printf("  -kernel          Upgrade kernel\n");
    printf("  -rootfs          Upgrade root filesystem\n");
    printf("  -appfs           Upgrade application filesystem\n");
    printf("\n");
    printf("Options:\n");
    printf("  -f, --format <fmt>   Image format (raw, ext4, ext4-sparse, squashfs)\n");
    printf("  -F, --force          Force upgrade without confirmation\n");
    printf("  -v, --verify         Verify image before upgrade\n");
    printf("  -h, --help           Display this help and exit\n");
    printf("  -V, --version        Display version information and exit\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s factory.bin                    # Full package upgrade\n", prog);
    printf("  %s -kernel zImage                # Kernel upgrade only\n", prog);
    printf("  %s -rootfs rootfs.ext4 -f ext4   # RootFS upgrade with format\n", prog);
    printf("\n");
}

int main(int argc, char *argv[])
{
    int opt;
    int option_index = 0;
    UPGRADE_CTX_T ctx;
    int ret;
    bool type_set = false;

    /* 特殊处理：检查是否是整包升级（factory.bin） */
    if (argc == 2) {
        char *filename = basename(argv[1]);
        if (strcmp(filename, "factory.bin") == 0) {
            /* 检查root权限 */
            if (getuid() != 0) {
                fprintf(stderr, "Error: This tool must be run as root\n");
                return 1;
            }

            /* 检查文件是否存在 */
            if (access(argv[1], F_OK) != 0) {
                fprintf(stderr, "Error: File not found: %s\n", argv[1]);
                return 1;
            }

            printf("\n");
            printf("========================================\n");
            printf("  Factory Package Upgrade\n");
            printf("========================================\n");
            printf("\n");

            /* 执行整包升级 */
            ret = mkkimg_upgrade_package(argv[1]);
            printf("\n");
            return (ret == 0) ? 0 : 1;
        }
    }

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
        opt = getopt_long(argc, argv, "l:u:k:t:d:r:a:f:FvhV", long_options, &option_index);

        if (opt == -1)
            break;

        switch (opt) {
            /* 升级类型 */
            case 'l':  /* loader */
                ctx.type = UPGRADE_TYPE_LOADER;
                strncpy(ctx.image_path, optarg, sizeof(ctx.image_path) - 1);
                type_set = true;
                break;
            case 'u':  /* uboot */
                ctx.type = UPGRADE_TYPE_UBOOT;
                strncpy(ctx.image_path, optarg, sizeof(ctx.image_path) - 1);
                type_set = true;
                break;
            case 'k':  /* kernel */
                ctx.type = UPGRADE_TYPE_KERNEL;
                strncpy(ctx.image_path, optarg, sizeof(ctx.image_path) - 1);
                type_set = true;
                break;
            case 't':  /* teeos */
                ctx.type = UPGRADE_TYPE_TEEOS;
                strncpy(ctx.image_path, optarg, sizeof(ctx.image_path) - 1);
                type_set = true;
                break;
            case 'd':  /* fdt/dtb */
                ctx.type = UPGRADE_TYPE_FDT;
                strncpy(ctx.image_path, optarg, sizeof(ctx.image_path) - 1);
                type_set = true;
                break;
            case 'r':  /* rootfs */
                ctx.type = UPGRADE_TYPE_ROOTFS;
                strncpy(ctx.image_path, optarg, sizeof(ctx.image_path) - 1);
                type_set = true;
                break;
            case 'a':  /* appfs */
                ctx.type = UPGRADE_TYPE_APPFS;
                strncpy(ctx.image_path, optarg, sizeof(ctx.image_path) - 1);
                type_set = true;
                break;

            /* 选项 */
            case 'f':  /* format */
                if (strcmp(optarg, "raw") == 0)
                    ctx.format = IMAGE_FORMAT_RAW;
                else if (strcmp(optarg, "ext4") == 0)
                    ctx.format = IMAGE_FORMAT_EXT4;
                else if (strcmp(optarg, "ext4-sparse") == 0)
                    ctx.format = IMAGE_FORMAT_EXT4_SPARSE;
                else if (strcmp(optarg, "squashfs") == 0)
                    ctx.format = IMAGE_FORMAT_SQUASHFS;
                else {
                    fprintf(stderr, "Unknown format: %s\n", optarg);
                    return 1;
                }
                break;
            case 'F':  /* force */
                ctx.force = true;
                break;
            case 'v':  /* verify */
                ctx.verify = true;
                break;
            case 'V':  /* version */
                printf("EMS Upgrade Tool v%s\n", UPGRADE_VERSION);
                return 0;
            case 'h':  /* help */
            default:
                print_usage(argv[0]);
                return (opt == 'h') ? 0 : 1;
        }
    }

    /* 检查是否指定了升级类型 */
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

    /* 使用配置驱动的通用升级流程 */
    char *filename = basename(ctx.image_path);
    ret = upgrade_execute_generic(filename, ctx.image_path, progress_callback);
    if (ret < 0) {
        fprintf(stderr, "\nUpgrade failed: %d\n", ret);
        upgrade_cleanup(&ctx);
        return 1;
    }

    /* 清理 */
    upgrade_cleanup(&ctx);

    return 0;
}
