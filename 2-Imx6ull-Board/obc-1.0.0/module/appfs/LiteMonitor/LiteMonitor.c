#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
#include <ctype.h>
#include <time.h>
#include <ifaddrs.h>
#include <net/if.h>

/* ==================== 配置常量 ==================== */
#define BROADCAST_IP "192.168.18.255"
#define BROADCAST_PORT 5005
#define INTERVAL_SEC 2

/* ==================== 数据结构定义 ==================== */

/* 基础设备信息 */
typedef struct {
    char device_name[64];    /* 设备名称 */
    char ip_address[32];     /* IP地址 */
} sys_info_base_t;

/* CPU信息 */
typedef struct {
    float usage_percent;     /* CPU使用率(%) */
    float temperature;       /* CPU温度(°C) */
    int core_count;          /* CPU核心数 */
} sys_info_cpu_t;

/* 内存信息 */
typedef struct {
    float usage_percent;     /* 内存占用率(%) */
} sys_info_mem_t;

/* GPU信息 */
typedef struct {
    int has_gpu;             /* 是否存在GPU (0:无, 1:有) */
    float usage_percent;     /* GPU使用率(%) */
    float temperature;       /* GPU温度(°C) */
    float mem_usage_percent; /* 显存占用率(%) */
} sys_info_gpu_t;

/* 网络信息 */
typedef struct {
    char upload_speed[32];   /* 上行带宽 */
    char download_speed[32]; /* 下行带宽 */
} sys_info_net_t;

/* 单个设备完整信息 */
typedef struct {
    sys_info_base_t base;    /* 基础信息 */
    sys_info_cpu_t cpu;      /* CPU信息 */
    sys_info_mem_t mem;      /* 内存信息 */
    sys_info_gpu_t gpu;      /* GPU信息 */
    sys_info_net_t net;      /* 网络信息 */
} sys_info_single_t;

/* 网络统计 */
typedef struct {
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
} NetStats;

/* ==================== 函数声明 ==================== */
void get_hostname(char *name, size_t size);
void get_ip_address(char *ip, size_t size);
float get_cpu_usage(void);
float get_cpu_temp(void);
int get_cpu_cores(void);
float get_mem_usage(void);
NetStats get_net_stats(void);
void format_size(unsigned long long bytes, char *buffer, size_t size);
void send_udp_broadcast(const sys_info_single_t *info);
void collect_system_info(sys_info_single_t *info, NetStats *prev_net);

/* ==================== 主函数 ==================== */

int main(void)
{
    sys_info_single_t sys_info;
    NetStats prev_net;

    printf("LiteMonitor - 系统监控广播服务\n");
    printf("广播地址: %s:%d\n", BROADCAST_IP, BROADCAST_PORT);
    printf("发送间隔: %d 秒\n\n", INTERVAL_SEC);

    /* 预热网络统计（避免第一次计算出现异常值） */
    printf("正在校准网络带宽...\n");
    prev_net = get_net_stats();
    sleep(1);

    while (1) {
        /* 收集系统信息 */
        collect_system_info(&sys_info, &prev_net);

        /* 发送UDP广播 */
        send_udp_broadcast(&sys_info);

        /* 调试输出 */
        printf("--- [%s] %s ---\n", sys_info.base.ip_address, sys_info.base.device_name);
        printf("CPU: %.1f%% | Temp: %.1f°C | Cores: %d\n",
               sys_info.cpu.usage_percent, sys_info.cpu.temperature, sys_info.cpu.core_count);
        printf("MEM: %.1f%%\n", sys_info.mem.usage_percent);
        printf("GPU: %s\n", sys_info.gpu.has_gpu ? "Available" : "N/A");
        printf("NET: ↑%s  ↓%s\n", sys_info.net.upload_speed, sys_info.net.download_speed);
        printf("----------------------------\n\n");

        sleep(INTERVAL_SEC);
    }

    return 0;
}

/* ==================== 函数实现 ==================== */

/**
 * @brief 收集所有系统信息
 */
void collect_system_info(sys_info_single_t *info, NetStats *prev_net)
{
    NetStats curr_net;
    unsigned long long up_diff, down_diff;

    if (info == NULL || prev_net == NULL) {
        return;
    }

    memset(info, 0, sizeof(sys_info_single_t));

    /* 基础信息 */
    get_hostname(info->base.device_name, sizeof(info->base.device_name));
    get_ip_address(info->base.ip_address, sizeof(info->base.ip_address));

    /* CPU信息 */
    info->cpu.usage_percent = get_cpu_usage();
    info->cpu.temperature = get_cpu_temp();
    info->cpu.core_count = get_cpu_cores();

    /* 内存信息 */
    info->mem.usage_percent = get_mem_usage();

    /* GPU信息（i.MX6ULL无独立GPU） */
    info->gpu.has_gpu = 0;
    info->gpu.usage_percent = 0.0f;
    info->gpu.temperature = 0.0f;
    info->gpu.mem_usage_percent = 0.0f;

    /* 网络信息 */
    curr_net = get_net_stats();

    /* 防止下溢（网卡重置导致数值变小） */
    if (curr_net.rx_bytes >= prev_net->rx_bytes) {
        down_diff = curr_net.rx_bytes - prev_net->rx_bytes;
    } else {
        down_diff = 0;
    }

    if (curr_net.tx_bytes >= prev_net->tx_bytes) {
        up_diff = curr_net.tx_bytes - prev_net->tx_bytes;
    } else {
        up_diff = 0;
    }

    unsigned long long down_rate = down_diff / INTERVAL_SEC;
    unsigned long long up_rate = up_diff / INTERVAL_SEC;

    format_size(up_rate, info->net.upload_speed, sizeof(info->net.upload_speed));
    format_size(down_rate, info->net.download_speed, sizeof(info->net.download_speed));

    /* 更新网络统计 */
    *prev_net = curr_net;
}

/**
 * @brief 获取主机名
 */
void get_hostname(char *name, size_t size)
{
    if (name == NULL || size == 0) {
        return;
    }

    if (gethostname(name, size) != 0) {
        strncpy(name, "unknown", size - 1);
        name[size - 1] = '\0';
    }
}

/**
 * @brief 获取IP地址（排除回环地址）
 */
void get_ip_address(char *ip, size_t size)
{
    struct ifaddrs *ifaddr, *ifa;
    int family;
    char host[NI_MAXHOST];

    if (ip == NULL || size == 0) {
        return;
    }

    strncpy(ip, "0.0.0.0", size - 1);
    ip[size - 1] = '\0';

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {
            continue;
        }

        family = ifa->ifa_addr->sa_family;

        /* 只获取IPv4地址，排除回环地址 */
        if (family == AF_INET && !(ifa->ifa_flags & IFF_LOOPBACK)) {
            int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                                host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s == 0) {
                strncpy(ip, host, size - 1);
                ip[size - 1] = '\0';
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
}

/**
 * @brief 获取CPU使用率
 */
float get_cpu_usage(void)
{
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) {
        return 0.0f;
    }

    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    if (fscanf(fp, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) != 8) {
        fclose(fp);
        return 0.0f;
    }

    unsigned long long total1 = user + nice + system + idle + iowait + irq + softirq + steal;
    unsigned long long idle1 = idle + iowait;

    fclose(fp);

    /* 短暂等待 */
    usleep(100000); // 100ms

    fp = fopen("/proc/stat", "r");
    if (!fp) {
        return 0.0f;
    }

    if (fscanf(fp, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) != 8) {
        fclose(fp);
        return 0.0f;
    }

    unsigned long long total2 = user + nice + system + idle + iowait + irq + softirq + steal;
    unsigned long long idle2 = idle + iowait;

    fclose(fp);

    unsigned long long total_diff = total2 - total1;
    unsigned long long idle_diff = idle2 - idle1;

    if (total_diff == 0) {
        return 0.0f;
    }

    return (float)(total_diff - idle_diff) * 100.0f / (float)total_diff;
}

/**
 * @brief 获取CPU温度（适配i.MX6ULL）
 */
float get_cpu_temp(void)
{
    FILE *fp;
    int temp_millidegrees;
    float temp = 0.0f;

    /* 尝试读取thermal_zone0（i.MX6ULL常用位置） */
    fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (fp) {
        if (fscanf(fp, "%d", &temp_millidegrees) == 1) {
            temp = temp_millidegrees / 1000.0f;
        }
        fclose(fp);
        return temp;
    }

    /* 备用：尝试hwmon目录 */
    DIR *dir = opendir("/sys/class/hwmon");
    if (!dir) {
        return 0.0f;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        char path[512];
        snprintf(path, sizeof(path), "/sys/class/hwmon/%s/temp1_input", entry->d_name);

        fp = fopen(path, "r");
        if (fp) {
            int val;
            if (fscanf(fp, "%d", &val) == 1) {
                temp = val / 1000.0f;
                fclose(fp);
                break;
            }
            fclose(fp);
        }
    }

    closedir(dir);
    return temp;
}

/**
 * @brief 获取CPU核心数
 */
int get_cpu_cores(void)
{
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) {
        return 1;
    }

    int count = 0;
    char line[256];

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "processor", 9) == 0) {
            count++;
        }
    }

    fclose(fp);
    return (count > 0) ? count : 1;
}

/**
 * @brief 获取内存使用率
 */
float get_mem_usage(void)
{
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        return 0.0f;
    }

    unsigned long long mem_total = 0, mem_available = 0;
    char line[256];

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %llu kB", &mem_total) == 1) {
            continue;
        }
        if (sscanf(line, "MemAvailable: %llu kB", &mem_available) == 1) {
            continue;
        }
    }

    fclose(fp);

    if (mem_total == 0) {
        return 0.0f;
    }

    return (float)(mem_total - mem_available) * 100.0f / (float)mem_total;
}

/**
 * @brief 获取网络统计（所有非回环接口的总和）
 */
NetStats get_net_stats(void)
{
    NetStats stats = {0, 0};
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp) {
        return stats;
    }

    char line[256];
    /* 跳过前两行表头 */
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return stats;
    }
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return stats;
    }

    while (fgets(line, sizeof(line), fp)) {
        /* 跳过回环接口 */
        if (strstr(line, "lo:") != NULL) {
            continue;
        }

        char *iface = strtok(line, ":");
        if (iface) {
            /* 去除接口名前的空格 */
            while (*iface && isspace(*iface)) {
                iface++;
            }

            if (strlen(iface) > 0) {
                unsigned long long rx, tx;
                /* 格式：rx_bytes rx_packets ... tx_bytes tx_packets ... */
                if (sscanf(strtok(NULL, ":"), "%llu %*d %*d %*d %*d %*d %*d %*d %llu",
                           &rx, &tx) == 2) {
                    stats.rx_bytes += rx;
                    stats.tx_bytes += tx;
                }
            }
        }
    }

    fclose(fp);
    return stats;
}

/**
 * @brief 格式化字节大小为可读字符串
 */
void format_size(unsigned long long bytes, char *buffer, size_t size)
{
    if (buffer == NULL || size == 0) {
        return;
    }

    if (bytes >= 1024ULL * 1024ULL) {
        snprintf(buffer, size, "%.1fMB/s", (float)bytes / (1024.0f * 1024.0f));
    } else if (bytes >= 1024ULL) {
        snprintf(buffer, size, "%.1fKB/s", (float)bytes / 1024.0f);
    } else {
        snprintf(buffer, size, "%lluB/s", bytes);
    }
}

/**
 * @brief 发送UDP广播
 */
void send_udp_broadcast(const sys_info_single_t *info)
{
    int sock;
    struct sockaddr_in broadcast_addr;
    int broadcast_enable = 1;

    if (info == NULL) {
        return;
    }

    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        perror("socket");
        return;
    }

    /* 启用广播 */
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST,
                   &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("setsockopt");
        close(sock);
        return;
    }

    /* 设置广播地址 */
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(BROADCAST_PORT);
    broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    /* 发送结构体数据 */
    if (sendto(sock, info, sizeof(sys_info_single_t), 0,
               (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr)) < 0) {
        perror("sendto");
    }

    close(sock);
}
