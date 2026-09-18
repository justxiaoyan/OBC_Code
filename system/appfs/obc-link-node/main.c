/*
 * OBC-Link Node - 主程序
 * 用于接收上位机配置并进行网络通信测试
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include "tcp_server.h"
#include "config_parser.h"

#define VERSION "1.0.0"
#define DEFAULT_PORT 8888

static int running = 1;
static tcp_server_t *server = NULL;

// 信号处理函数
void signal_handler(int signum) {
    printf("\n[INFO] 收到信号 %d, 正在关闭...\n", signum);
    running = 0;
    if (server) {
        tcp_server_stop(server);
    }
}

// 打印使用说明
void print_usage(const char *program) {
    printf("OBC-Link Node v%s\n", VERSION);
    printf("用法: %s [选项]\n\n", program);
    printf("选项:\n");
    printf("  -p, --port PORT        监听端口 (默认: %d)\n", DEFAULT_PORT);
    printf("  -h, --help             显示此帮助信息\n");
    printf("  -v, --version          显示版本信息\n");
    printf("\n示例:\n");
    printf("  %s                    使用默认端口 %d\n", program, DEFAULT_PORT);
    printf("  %s -p 9999           使用端口 9999\n", program);
    printf("\n");
}

// 数据接收回调函数
void on_data_received(const char *data, size_t len) {
    printf("\n[接收] 收到数据 (%zu 字节):\n", len);
    printf("----------------------------------------\n");
    printf("%.*s\n", (int)len, data);
    printf("----------------------------------------\n");

    // 尝试解析为 JSON 配置
    device_config_t config;
    if (parse_config_json(data, &config) == 0) {
        printf("\n[解析] 配置解析成功:\n");
        print_config(&config);

        // 释放配置内存
        free_config(&config);
    } else {
        printf("[警告] 无法解析为有效的 JSON 配置\n");
    }
    printf("\n");
}

// 客户端连接回调
void on_client_connected(const char *client_ip, int client_port) {
    printf("\n[连接] 客户端已连接: %s:%d\n", client_ip, client_port);
}

// 客户端断开回调
void on_client_disconnected(void) {
    printf("\n[断开] 客户端已断开连接\n");
}

int main(int argc, char *argv[]) {
    int port = DEFAULT_PORT;
    int opt;

    // 长选项定义
    static struct option long_options[] = {
        {"port",    required_argument, 0, 'p'},
        {"help",    no_argument,       0, 'h'},
        {"version", no_argument,       0, 'v'},
        {0, 0, 0, 0}
    };

    // 解析命令行参数
    while ((opt = getopt_long(argc, argv, "p:hv", long_options, NULL)) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                if (port <= 0 || port > 65535) {
                    fprintf(stderr, "错误: 无效的端口号 %d\n", port);
                    return EXIT_FAILURE;
                }
                break;
            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            case 'v':
                printf("OBC-Link Node v%s\n", VERSION);
                return EXIT_SUCCESS;
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    // 注册信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("====================================\n");
    printf("  OBC-Link Node v%s\n", VERSION);
    printf("====================================\n\n");

    // 创建并启动 TCP 服务器
    server = tcp_server_create(port);
    if (!server) {
        fprintf(stderr, "错误: 创建 TCP 服务器失败\n");
        return EXIT_FAILURE;
    }

    // 设置回调函数
    tcp_server_set_callbacks(server, on_data_received,
                            on_client_connected, on_client_disconnected);

    if (tcp_server_start(server) != 0) {
        fprintf(stderr, "错误: 启动 TCP 服务器失败\n");
        tcp_server_destroy(server);
        return EXIT_FAILURE;
    }

    printf("[启动] TCP 服务器正在监听端口 %d\n", port);
    printf("[提示] 按 Ctrl+C 退出程序\n\n");

    // 主循环
    while (running) {
        tcp_server_process(server);
        usleep(10000); // 10ms
    }

    // 清理资源
    printf("\n[清理] 正在关闭服务器...\n");
    tcp_server_destroy(server);
    printf("[完成] 服务器已关闭\n");

    return EXIT_SUCCESS;
}
