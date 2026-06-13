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
#include <nvml.h>

// --- 配置常量 ---
#define BROADCAST_IP "10.10.0.255"
#define BROADCAST_PORT 5005
#define INTERVAL_SEC 2
#define BUFFER_SIZE 1024

// --- 数据结构 ---
typedef struct {
    unsigned long long rx_bytes;
    unsigned long long tx_bytes;
} NetStats;

// --- 函数声明 ---
char* get_hostname();
char* get_ip_address();
float get_cpu_usage();
float get_cpu_temp();
float get_mem_usage();
int init_gpu(); // 返回 0 表示成功，-1 表示失败
void get_gpu_stats(float *usage, float *temp, float *mem_usage);
NetStats get_net_stats();
void send_udp_broadcast(const char *message);
void format_size(unsigned long long bytes, char *buffer);

int main() {
    char message[BUFFER_SIZE];
    char net_up_str[32];
    char net_down_str[32];
    NetStats prev_net, curr_net;
    unsigned long long up_diff, down_diff;
    
    // 1. 初始化 GPU
    int gpu_available = init_gpu();

    printf("开始监控系统...\n");
    printf("广播地址: %s:%d\n", BROADCAST_IP, BROADCAST_PORT);
    if (gpu_available != 0) {
        printf("警告: GPU 不可用，相关数据将显示为 0\n");
    }

    // 2. 预热网络统计（避免第一次循环计算出巨大的带宽）
    printf("正在校准网络带宽...\n");
    prev_net = get_net_stats();
    sleep(1);

    while (1) {
        // --- 收集数据 ---
        char *hostname = get_hostname();
        char *ip = get_ip_address();
        float cpu_usage = get_cpu_usage();
        float cpu_temp = get_cpu_temp();
        float mem_usage = get_mem_usage();

        float gpu_usage = 0.0f, gpu_temp = 0.0f, gpu_mem_usage = 0.0f;
        if (gpu_available == 0) {
            get_gpu_stats(&gpu_usage, &gpu_temp, &gpu_mem_usage);
        }

        // --- 计算网络带宽 ---
        curr_net = get_net_stats();
        
        // 防止下溢（例如网卡重置导致数值变小）
        if (curr_net.rx_bytes >= prev_net.rx_bytes) {
            down_diff = curr_net.rx_bytes - prev_net.rx_bytes;
        } else {
            down_diff = 0;
        }
        
        if (curr_net.tx_bytes >= prev_net.tx_bytes) {
            up_diff = curr_net.tx_bytes - prev_net.tx_bytes;
        } else {
            up_diff = 0;
        }

        unsigned long long down_rate = down_diff / INTERVAL_SEC;
        unsigned long long up_rate = up_diff / INTERVAL_SEC;

        format_size(up_rate, net_up_str);
        format_size(down_rate, net_down_str);

        // --- 组装消息 ---
        snprintf(message, sizeof(message),
            "PCName:%s\n"
            "IP:%s\n"
            "CPU:%.1f%%\n"
            "CPUTemp:%.1f℃\n"
            "MEM:%.1f%%\n"
            "GPU:%.0f%%\n"
            "GPUTemp:%.1f℃\n"
            "GPUMEM:%.1f%%\n"
            "Netup:%s\n"
            "NetDonw:%s",
            hostname, ip, cpu_usage, cpu_temp, mem_usage,
            gpu_usage, gpu_temp, gpu_mem_usage,
            net_up_str, net_down_str
        );

        // --- 发送广播 ---
        send_udp_broadcast(message);

        // --- 调试输出 ---
        printf("--- 发送数据 ---\n%s\n----------------\n", message);

        // --- 更新状态 ---
        prev_net = curr_net;
        
        free(hostname);
        free(ip);
        
        sleep(INTERVAL_SEC);
    }

    if (gpu_available == 0) {
        nvmlShutdown();
    }
    return 0;
}

// --- 函数实现 ---

// 初始化 GPU (独立出来以便在 main 中判断)
int init_gpu() {
    nvmlReturn_t result = nvmlInit();
    if (result != NVML_SUCCESS) {
        // 打印具体错误信息
        printf("NVML 初始化失败: %s\n", nvmlErrorString(result));
        return -1;
    }
    // 尝试获取设备句柄，确认显卡存在
    nvmlDevice_t device;
    result = nvmlDeviceGetHandleByIndex(0, &device);
    if (result != NVML_SUCCESS) {
        printf("未找到 NVIDIA 设备: %s\n", nvmlErrorString(result));
        nvmlShutdown();
        return -1;
    }
    return 0;
}

char* get_hostname() {
    char *name = (char*)malloc(256);
    if (gethostname(name, 256) != 0) {
        strcpy(name, "unknown");
    }
    return name;
}

char* get_ip_address() {
    char *ip = (char*)malloc(64);
    strcpy(ip, "0.0.0.0");

    struct ifaddrs *ifaddr, *ifa;
    int family;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return ip;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        if (family == AF_INET && !(ifa->ifa_flags & IFF_LOOPBACK)) {
            int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                continue;
            }
            strcpy(ip, host);
            break;
        }
    }

    freeifaddrs(ifaddr);
    return ip;
}

float get_cpu_usage() {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return 0.0f;

    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    if (fscanf(fp, "cpu %llu %llu %llu %llu %llu %llu %llu %llu", 
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) == 8) {
        
        unsigned long long total1 = user + nice + system + idle + iowait + irq + softirq + steal;
        unsigned long long idle1 = idle + iowait;

        fclose(fp);
        sleep(1);

        fp = fopen("/proc/stat", "r");
        if (fp && fscanf(fp, "cpu %llu %llu %llu %llu %llu %llu %llu %llu", 
               &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal) == 8) {
            
            unsigned long long total2 = user + nice + system + idle + iowait + irq + softirq + steal;
            unsigned long long idle2 = idle + iowait;

            fclose(fp);

            unsigned long long total_diff = total2 - total1;
            unsigned long long idle_diff = idle2 - idle1;

            if (total_diff == 0) return 0.0f;
            return (float)(total_diff - idle_diff) * 100.0f / (float)total_diff;
        }
    }
    if (fp) fclose(fp);
    return 0.0f;
}

float get_cpu_temp() {
    DIR *dir = opendir("/sys/class/hwmon");
    if (!dir) return 0.0f;

    struct dirent *entry;
    float temp = 0.0f;
    
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        
        char path[512]; 
        snprintf(path, sizeof(path), "/sys/class/hwmon/%s/name", entry->d_name);
        
        FILE *name_fp = fopen(path, "r");
        if (name_fp) {
            char name[64];
            if (fgets(name, sizeof(name), name_fp)) {
                if (strstr(name, "coretemp") != NULL) {
                    char temp_path[512];
                    snprintf(temp_path, sizeof(temp_path), "/sys/class/hwmon/%s/temp1_input", entry->d_name);
                    FILE *temp_fp = fopen(temp_path, "r");
                    if (temp_fp) {
                        int val;
                        if (fscanf(temp_fp, "%d", &val) == 1) {
                            temp = val / 1000.0f;
                            fclose(temp_fp);
                            fclose(name_fp);
                            closedir(dir);
                            return temp;
                        }
                        fclose(temp_fp);
                    }
                }
            }
            fclose(name_fp);
        }
    }
    closedir(dir);
    return 0.0f;
}

float get_mem_usage() {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) return 0.0f;

    unsigned long long mem_total = 0, mem_available = 0;
    char line[256];
    
    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "MemTotal: %llu kB", &mem_total) == 1) continue;
        if (sscanf(line, "MemAvailable: %llu kB", &mem_available) == 1) continue;
    }
    fclose(fp);

    if (mem_total == 0) return 0.0f;
    return (float)(mem_total - mem_available) * 100.0f / (float)mem_total;
}

void get_gpu_stats(float *usage, float *temp, float *mem_usage) {
    nvmlDevice_t device;
    nvmlReturn_t result = nvmlDeviceGetHandleByIndex(0, &device);
    if (result != NVML_SUCCESS) {
        return;
    }

    nvmlUtilization_t util;
    result = nvmlDeviceGetUtilizationRates(device, &util);
    if (result == NVML_SUCCESS) {
        *usage = (float)util.gpu;
    }

    unsigned int temperature = 0;
    nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temperature);
    *temp = (float)temperature;

    nvmlMemory_t memory;
    nvmlDeviceGetMemoryInfo(device, &memory);
    if (memory.total > 0) {
        *mem_usage = (float)memory.used * 100.0f / (float)memory.total;
    }
}

NetStats get_net_stats() {
    NetStats stats = {0, 0};
    FILE *fp = fopen("/proc/net/dev", "r");
    if (!fp) return stats;

    char line[256];
    fgets(line, sizeof(line), fp);
    fgets(line, sizeof(line), fp);

    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "lo:") != NULL) continue;

        char *iface = strtok(line, ":");
        if (iface) {
            while(*iface && isspace(*iface)) iface++;
            if (strlen(iface) > 0) {
                unsigned long long rx, tx;
                if (sscanf(strtok(NULL, ":"), "%llu %*d %*d %*d %*d %*d %*d %*d %llu", &rx, &tx) == 2) {
                    stats.rx_bytes += rx;
                    stats.tx_bytes += tx;
                }
            }
        }
    }
    fclose(fp);
    return stats;
}

void format_size(unsigned long long bytes, char *buffer) {
    if (bytes >= 1024 * 1024) {
        sprintf(buffer, "%.1fM/s", (float)bytes / (1024 * 1024));
    } else if (bytes >= 1024) {
        sprintf(buffer, "%.1fKB/s", (float)bytes / 1024);
    } else {
        sprintf(buffer, "%lluB/s", bytes);
    }
}

void send_udp_broadcast(const char *message) {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        perror("socket");
        return;
    }

    int broadcast_enable = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable));

    struct sockaddr_in broadcast_addr;
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(BROADCAST_PORT);
    broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    sendto(sock, message, strlen(message), 0, 
           (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));

    close(sock);
}