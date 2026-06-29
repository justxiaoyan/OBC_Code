/*******************************************************************************
 * AM62x Memory Statistics Tool - System Memory Collection Implementation
 * 系统内存采集模块实现
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <stdint.h>
#include "sys_mem.h"

#define PROC_MEMINFO "/proc/meminfo"
#define PROC_IOMEM   "/proc/iomem"
#define PROC_ZONEINFO "/proc/zoneinfo"

/*******************************************************************************
 * 采集系统内存信息
 ******************************************************************************/
int collect_system_memory(system_mem_info_t *info) {
    FILE *fp;
    char line[256];
    char key[64];
    mem_size_t value;

    if (!info) return -1;
    memset(info, 0, sizeof(system_mem_info_t));

    fp = fopen(PROC_MEMINFO, "r");
    if (!fp) {
        fprintf(stderr, "Failed to open %s: %s\n", PROC_MEMINFO, strerror(errno));
        return -1;
    }

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%s %lu kB", key, &value) == 2) {
            if (strcmp(key, "MemTotal:") == 0) {
                info->total = value;
            } else if (strcmp(key, "MemFree:") == 0) {
                info->free = value;
            } else if (strcmp(key, "MemAvailable:") == 0) {
                info->available = value;
            } else if (strcmp(key, "Buffers:") == 0) {
                info->buffers = value;
            } else if (strcmp(key, "Cached:") == 0) {
                info->cached = value;
            } else if (strcmp(key, "Slab:") == 0) {
                info->slab = value;
            } else if (strcmp(key, "KernelStack:") == 0) {
                info->kernel_stack = value;
            } else if (strcmp(key, "PageTables:") == 0) {
                info->page_tables = value;
            } else if (strcmp(key, "VmallocUsed:") == 0) {
                info->vmalloc_used = value;
            }
        }
    }

    fclose(fp);

    /* 计算已使用内存 */
    info->used = info->total - info->free;

    return 0;
}

/*******************************************************************************
 * 采集内核内存信息
 ******************************************************************************/
int collect_kernel_memory(kernel_mem_info_t *kernel, const system_mem_info_t *sys) {
    if (!kernel || !sys) return -1;

    kernel->slab = sys->slab;
    kernel->kernel_stack = sys->kernel_stack;
    kernel->page_tables = sys->page_tables;
    kernel->vmalloc_used = sys->vmalloc_used;
    kernel->total = kernel->slab + kernel->kernel_stack +
                    kernel->page_tables + kernel->vmalloc_used;

    return 0;
}

/*******************************************************************************
 * 采集保留内存信息 - 从设备树读取
 ******************************************************************************/
int collect_reserved_memory(reserved_mem_info_t *reserved) {
    DIR *dir;
    struct dirent *entry;
    const char *dt_path = "/proc/device-tree/reserved-memory";

    if (!reserved) return -1;
    memset(reserved, 0, sizeof(reserved_mem_info_t));

    /* 先尝试从设备树读取 */
    dir = opendir(dt_path);
    if (dir) {
        while ((entry = readdir(dir)) != NULL && reserved->count < MAX_RESERVED_REGIONS) {
            /* 跳过特殊目录和属性文件 */
            if (entry->d_name[0] == '.' || entry->d_name[0] == '#' ||
                strcmp(entry->d_name, "name") == 0 ||
                strcmp(entry->d_name, "ranges") == 0 ||
                strcmp(entry->d_name, "phandle") == 0) {
                continue;
            }

            char reg_path[512];
            snprintf(reg_path, sizeof(reg_path), "%s/%s/reg", dt_path, entry->d_name);

            FILE *fp = fopen(reg_path, "rb");
            if (fp) {
                unsigned char buf[16];
                memset(buf, 0, sizeof(buf));
                size_t n = fread(buf, 1, 16, fp);
                fclose(fp);

                if (n >= 8) {  /* 至少需要8字节（地址部分） */
                    /* 解析设备树的reg属性 (big-endian) */
                    uint64_t addr, size;

                    if (n == 16) {
                        /* 64位地址 + 64位大小 */
                        addr = ((uint64_t)buf[0] << 56) | ((uint64_t)buf[1] << 48) |
                               ((uint64_t)buf[2] << 40) | ((uint64_t)buf[3] << 32) |
                               ((uint64_t)buf[4] << 24) | ((uint64_t)buf[5] << 16) |
                               ((uint64_t)buf[6] << 8)  | ((uint64_t)buf[7]);

                        size = ((uint64_t)buf[8] << 56) | ((uint64_t)buf[9] << 48) |
                               ((uint64_t)buf[10] << 40) | ((uint64_t)buf[11] << 32) |
                               ((uint64_t)buf[12] << 24) | ((uint64_t)buf[13] << 16) |
                               ((uint64_t)buf[14] << 8)  | ((uint64_t)buf[15]);

                        /* 调试输出 */
                        #ifdef DEBUG_REG_PARSE
                        fprintf(stderr, "DEBUG: %s: n=%zu, addr=0x%lx, size=0x%lx (%lu bytes)\n",
                                entry->d_name, n, addr, size, size);
                        #endif
                    } else if (n == 8) {
                        /* 32位地址 + 32位大小 */
                        addr = ((uint64_t)buf[0] << 24) | ((uint64_t)buf[1] << 16) |
                               ((uint64_t)buf[2] << 8)  | ((uint64_t)buf[3]);

                        size = ((uint64_t)buf[4] << 24) | ((uint64_t)buf[5] << 16) |
                               ((uint64_t)buf[6] << 8)  | ((uint64_t)buf[7]);
                    } else {
                        /* 其他长度，跳过 */
                        continue;
                    }

                    /* 统计所有区域，包括size为0的（但只记录有大小的） */
                    if (size > 0) {
                        reserved_mem_region_t *region = &reserved->regions[reserved->count];
                        region->start_addr = addr;
                        region->end_addr = addr + size - 1;
                        /* 保存原始字节数，不要立即转换为KB，避免小于1024字节的被截断为0 */
                        region->size = size; /* 保持字节单位 */

                        /* 复制名称 (去掉@后的地址部分) */
                        size_t name_len = strlen(entry->d_name);
                        if (name_len >= MAX_NAME_LEN) {
                            name_len = MAX_NAME_LEN - 1;
                        }
                        memcpy(region->name, entry->d_name, name_len);
                        region->name[name_len] = '\0';

                        /* 去掉@后的地址 */
                        char *at = strchr(region->name, '@');
                        if (at) *at = '\0';

                        reserved->count++;
                    }
                }
            }
        }
        closedir(dir);
    }

    /* 如果设备树读取失败，回退到/proc/iomem */
    if (reserved->count == 0) {
        FILE *fp = fopen(PROC_IOMEM, "r");
        if (!fp) {
            fprintf(stderr, "Failed to open %s: %s\n", PROC_IOMEM, strerror(errno));
            return -1;
        }

        char line[512];
        while (fgets(line, sizeof(line), fp) && reserved->count < MAX_RESERVED_REGIONS) {
            char *colon = strchr(line, ':');
            if (!colon) continue;

            /* 检查是否是Reserved */
            if (strstr(line, "Reserved")) {
                reserved_mem_region_t *region = &reserved->regions[reserved->count];

                unsigned long long start, end;
                if (sscanf(line, "%llx-%llx", &start, &end) == 2) {
                    region->start_addr = start;
                    region->end_addr = end;
                    region->size = (end - start + 1) / 1024;

                    char *name_start = colon + 1;
                    while (*name_start == ' ') name_start++;
                    char *name_end = strchr(name_start, '\n');
                    if (name_end) *name_end = '\0';

                    strncpy(region->name, name_start, MAX_NAME_LEN - 1);
                    region->name[MAX_NAME_LEN - 1] = '\0';

                    reserved->count++;
                }
            }
        }
        fclose(fp);
    }

    return 0;
}


/*******************************************************************************
 * 计算内存健康状态
 ******************************************************************************/
void calculate_memory_health(memory_health_t *health, const system_mem_info_t *sys) {
    if (!health || !sys || sys->total == 0) return;

    health->usage_percent = (int)((sys->used * 100) / sys->total);
    health->available_percent = (int)((sys->available * 100) / sys->total);

    /* 判断健康状态 */
    if (health->usage_percent < 70) {
        health->status = MEM_HEALTH_GOOD;
    } else if (health->usage_percent < 85) {
        health->status = MEM_HEALTH_WARNING;
    } else {
        health->status = MEM_HEALTH_CRITICAL;
    }
}
