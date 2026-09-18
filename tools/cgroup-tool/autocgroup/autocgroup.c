#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>

#define MAX_PATH_LEN 256
#define MAX_BUFFER_LEN 1024

void print_usage(const char *program_name) {
    fprintf(stderr, "Usage: %s -n cgroup_name [-u cpu_percent] [-m max_memory high_memory] [-c cpuset] [-r read_bps] [-w write_bps]\n", program_name);
    fprintf(stderr, "  -n cgroup_name     : cgroup name\n");
    fprintf(stderr, "  -u cpu_percent     : CPU usage limit percentage (0-100)\n");
    fprintf(stderr, "  -p PID             : Add pid to cgroup\n");
    fprintf(stderr, "  -m max_memory high_memory : Memory limits in bytes\n");
    fprintf(stderr, "  -c cpuset          : CPU set (e.g., 0-3 or 1)\n");
    fprintf(stderr, "  -r read_bps        : Read bandwidth limit in bytes/sec\n");
    fprintf(stderr, "  -w write_bps       : Write bandwidth limit in bytes/sec\n");
    fprintf(stderr, "  -s show            : Show all config\n");
    fprintf(stderr, "Example:\n");
    fprintf(stderr, "  %s -n bsp_test -u 50\n", program_name);
    fprintf(stderr, "  %s -n bsp_test -p 123\n", program_name);
    fprintf(stderr, "  %s -n bsp_test -m 325058560 314572800\n", program_name);
    fprintf(stderr, "  %s -n bsp_test -c 0-1\n", program_name);
    fprintf(stderr, "  %s -n bsp_test -r 1048576 -w 1048576\n", program_name);
    fprintf(stderr, "  %s -n bsp_test -sn");
}

int create_cgroup_dir(const char *cgroup_name) {
    char cgroup_path[MAX_PATH_LEN];
    snprintf(cgroup_path, sizeof(cgroup_path), "/sys/fs/cgroup/%s", cgroup_name);

    if (access(cgroup_path, F_OK) != 0) {
        if (mkdir(cgroup_path, 0755) != 0) {
            perror("Failed to create cgroup directory");
            return -1;
        }
        printf("Created cgroup directory: %s\n", cgroup_path);
    }
    return 0;
}

int set_cpu_max(const char *cgroup_path, int cpu_percent) {
    char cpu_max_path[MAX_PATH_LEN];
    char buffer[MAX_BUFFER_LEN];
    int cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
    long long max_quota = cpu_percent * 1000;  // 100000 = 100% in cgroup v1
    long long period = 100000;
    long long quota = (cpu_count * max_quota);

    snprintf(cpu_max_path, sizeof(cpu_max_path), "%s/cpu.max", cgroup_path);
    snprintf(buffer, sizeof(buffer), "%lld %lld", quota, period);

    FILE *fp = fopen(cpu_max_path, "w");
    if (!fp) {
        perror("Failed to open cpu.max");
        return -1;
    }
    fprintf(fp, "%s\n", buffer);
    fclose(fp);
    printf("Set CPU max to: %s\n", buffer);
    return 0;
}

int set_memory_limit(const char *cgroup_path, long long max_memory, long long high_memory) {
    char memory_max_path[MAX_PATH_LEN];
    char memory_high_path[MAX_PATH_LEN];

    snprintf(memory_max_path, sizeof(memory_max_path), "%s/memory.max", cgroup_path);
    snprintf(memory_high_path, sizeof(memory_high_path), "%s/memory.high", cgroup_path);

    FILE *fp = fopen(memory_max_path, "w");
    if (!fp) {
        perror("Failed to open memory.max");
        return -1;
    }
    fprintf(fp, "%lld\n", max_memory);
    fclose(fp);
    printf("Set memory.max to: %lld bytes\n", max_memory);

    fp = fopen(memory_high_path, "w");
    if (!fp) {
        perror("Failed to open memory.high");
        return -1;
    }
    fprintf(fp, "%lld\n", high_memory);
    fclose(fp);
    printf("Set memory.high to: %lld bytes\n", high_memory);
    return 0;
}

int set_cpuset(const char *cgroup_path, const char *cpuset) {
    char cpuset_path[MAX_PATH_LEN];
    snprintf(cpuset_path, sizeof(cpuset_path), "%s/cpuset.cpus", cgroup_path);

    FILE *fp = fopen(cpuset_path, "w");
    if (!fp) {
        perror("Failed to open cpuset.cpus");
        return -1;
    }
    fprintf(fp, "%s\n", cpuset);
    fclose(fp);
    printf("Set cpuset.cpus to: %s\n", cpuset);
    return 0;
}

int set_io_limit(const char *cgroup_path, long long read_bps, long long write_bps) {
    char io_max_path[MAX_PATH_LEN];
    char buffer[MAX_BUFFER_LEN];

    snprintf(io_max_path, sizeof(io_max_path), "%s/io.max", cgroup_path);
    snprintf(buffer, sizeof(buffer), "179:0 rbps=%lld wbps=%lld riops=2048 wiops=2048", read_bps, write_bps);

    FILE *fp = fopen(io_max_path, "w");
    if (!fp) {
        perror("Failed to open io.max");
        return -1;
    }
    fprintf(fp, "%s\n", buffer);
    fclose(fp);
    printf("Set io.max to: %s\n", buffer);
    return 0;
}

int set_pid_procs(const char *cgroup_path, int pid)
{
    char cpu_max_path[MAX_PATH_LEN];
    char buffer[MAX_BUFFER_LEN];

    snprintf(cpu_max_path, sizeof(cpu_max_path), "%s/cgroup.procs", cgroup_path);
    snprintf(buffer, sizeof(buffer), "%d", pid);

    FILE *fp = fopen(cpu_max_path, "w");
    if (!fp) {
        perror("Failed to open cpu.max");
        return -1;
    }
    fprintf(fp, "%s\n", buffer);
    fclose(fp);
    printf("Set cgroup.proc to: %s\n", buffer);
    return 0;
}

// void show_config(const char *cgroup_name)
// {
//     char cgroup_path[MAX_PATH_LEN];
//     if (!cgroup_name) {
//         fprintf(stderr, "Error: cgroup name is required\n");
//         print_usage(argv[0]);
//         exit(EXIT_FAILURE);
//     }

//     if (create_cgroup_dir(cgroup_name) != 0) {
//         exit(EXIT_FAILURE);
//     }

//     snprintf(cgroup_path, sizeof(cgroup_path), "/sys/fs/cgroup/%s", cgroup_name);

//     if (cpu_percent >= 0) {
//         if (set_cpu_max(cgroup_path, cpu_percent) != 0) {
//             exit(EXIT_FAILURE);
//         }
//     }


// }

void table_disp(int *info_flag)
{
    if (*info_flag) {
        printf("\n");
    } 
    else {
        *info_flag = 1;
    }
    return ;
}

void show_cgroup_info(const char* cgroup_name) {
    char path[256];
    FILE *fp;
    int info_flag = 1;/* 1:表示没有参数 */

    // 显示 cgroup.procs
    snprintf(path, sizeof(path), "/sys/fs/cgroup/%s/cgroup.procs", cgroup_name);
    fp = fopen(path, "r");
    if (fp != NULL) {
        printf("cgroup.procs:\t");
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            printf("%s", line);
            info_flag = false;
        }
        fclose(fp);
    } else {
        perror("Error reading cgroup.procs");
    }
    table_disp(&info_flag);

    // 显示 cpu.max
    snprintf(path, sizeof(path), "/sys/fs/cgroup/%s/cpu.max", cgroup_name);
    fp = fopen(path, "r");
    if (fp != NULL) {
        printf("cpu.max:\t");
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            printf("%s", line);
            info_flag = false;
        }
        fclose(fp);
    } else {
        perror("Error reading cpu.max");
    }
    table_disp(&info_flag);
    // 显示 cpuset.cpus
    snprintf(path, sizeof(path), "/sys/fs/cgroup/%s/cpuset.cpus", cgroup_name);
    fp = fopen(path, "r");
    if (fp != NULL) {
        printf("cpuset.cpus:\t");
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            printf("%s", line);
            info_flag = false;
        }
        fclose(fp);
    } else {
        perror("Error reading cpuset.cpus");
    }
    table_disp(&info_flag);
    // 显示 io.max
    snprintf(path, sizeof(path), "/sys/fs/cgroup/%s/io.max", cgroup_name);
    fp = fopen(path, "r");
    if (fp != NULL) {
        printf("io.max:\t\t");
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            printf("%s", line);
            info_flag = false;
        }
        fclose(fp);
    } else {
        perror("Error reading io.max");
    }
    table_disp(&info_flag);
    // 显示 memory.max
    snprintf(path, sizeof(path), "/sys/fs/cgroup/%s/memory.max", cgroup_name);
    fp = fopen(path, "r");
    if (fp != NULL) {
        printf("memory.max:\t");
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            printf("%s", line);
            info_flag = false;
        }
        fclose(fp);
    } else {
        perror("Error reading memory.max");
    }
    table_disp(&info_flag);
    // 显示 memory.high
    snprintf(path, sizeof(path), "/sys/fs/cgroup/%s/memory.high", cgroup_name);
    fp = fopen(path, "r");
    if (fp != NULL) {
        printf("memory.high:\t");
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            printf("%s", line);
            info_flag = false;
        }
        fclose(fp);
    } else {
        perror("Error reading memory.high");
    }
    table_disp(&info_flag);

    // 显示 memory.high
    snprintf(path, sizeof(path), "/sys/fs/cgroup/%s/memory.current", cgroup_name);
    fp = fopen(path, "r");
    if (fp != NULL) {
        printf("memory.current:\t");
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            printf("%s", line);
            info_flag = false;
        }
        fclose(fp);
    } else {
        perror("Error reading memory.current");
    }
    table_disp(&info_flag);
    snprintf(path, sizeof(path), "/sys/fs/cgroup/%s/memory.events", cgroup_name);
    fp = fopen(path, "r");
    if (fp != NULL) {
        char line[256];
        while (fgets(line, sizeof(line), fp)) {
            printf("memory.events:\t");
            printf("%s", line);
            info_flag = false;
        }
        fclose(fp);
    } else {
        perror("Error reading memory.events");
    }
    table_disp(&info_flag);
}

int main(int argc, char *argv[]) {
    char *cgroup_name = NULL;
    int cpu_percent = -1;
    int pid = -1;
    long long max_memory = -1;
    long long high_memory = -1;
    char *cpuset = NULL;
    long long read_bps = -1;
    long long write_bps = -1;
    int opt;
    bool show_flag = false;

    while ((opt = getopt(argc, argv, "n:u:p:m:c:r:w:s:h")) != -1) {
        switch (opt) {
            case 'n':
                cgroup_name = optarg;
                break;
            case 'u':
                cpu_percent = atoi(optarg);
                break;
            case 'm':
                max_memory = atoll(argv[optind - 1]);
                high_memory = atoll(argv[optind]);
                optind +=1; // 跳过两个参数
                break;
            case 'c':
                cpuset = optarg;
                break;
            case 'r':
                read_bps = atoll(optarg);
                break;
            case 'w':
                write_bps = atoll(optarg);
                break;
            case 'p':
                pid = atoi(optarg);
                break;
            case 'h':
                print_usage(argv[0]);
                exit(EXIT_SUCCESS);
            case 's':
                show_flag = true;
                break;
            default:
                print_usage(argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (!cgroup_name) {
        fprintf(stderr, "Error: cgroup name is required\n");
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (create_cgroup_dir(cgroup_name) != 0) {
        exit(EXIT_FAILURE);
    }

    if (show_flag)
        goto show_info;

    char cgroup_path[MAX_PATH_LEN];
    snprintf(cgroup_path, sizeof(cgroup_path), "/sys/fs/cgroup/%s", cgroup_name);

    if (cpu_percent >= 0) {
        if (set_cpu_max(cgroup_path, cpu_percent) != 0) {
            exit(EXIT_FAILURE);
        }
    }

    if (pid >= 0) {
        if (set_pid_procs(cgroup_path, pid) != 0) {
            exit(EXIT_FAILURE);
        }
    }

    if (max_memory >= 0 && high_memory >= 0) {
        if (set_memory_limit(cgroup_path, max_memory, high_memory) != 0) {
            exit(EXIT_FAILURE);
        }
    }

    if (cpuset) {
        if (set_cpuset(cgroup_path, cpuset) != 0) {
            exit(EXIT_FAILURE);
        }
    }

    if (read_bps >= 0 && write_bps >= 0) {
        if (set_io_limit(cgroup_path, read_bps, write_bps) != 0) {
            exit(EXIT_FAILURE);
        }
    }

    return 0;

show_info:
    show_cgroup_info(cgroup_name);
    return 0;
}
