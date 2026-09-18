/*
 * TCP 服务器实现
 */

#include "tcp_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>

#define BUFFER_SIZE 4096
#define BACKLOG 5

// TCP 服务器结构体
struct tcp_server {
    int port;
    int listen_fd;
    int client_fd;
    char recv_buffer[BUFFER_SIZE];

    // 回调函数
    data_callback_t on_data;
    connected_callback_t on_connected;
    disconnected_callback_t on_disconnected;
};

// 设置非阻塞模式
static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

tcp_server_t* tcp_server_create(int port) {
    tcp_server_t *server = (tcp_server_t*)malloc(sizeof(tcp_server_t));
    if (!server) {
        return NULL;
    }

    memset(server, 0, sizeof(tcp_server_t));
    server->port = port;
    server->listen_fd = -1;
    server->client_fd = -1;

    return server;
}

void tcp_server_set_callbacks(tcp_server_t *server,
                              data_callback_t on_data,
                              connected_callback_t on_connected,
                              disconnected_callback_t on_disconnected) {
    if (!server) return;

    server->on_data = on_data;
    server->on_connected = on_connected;
    server->on_disconnected = on_disconnected;
}

int tcp_server_start(tcp_server_t *server) {
    if (!server) return -1;

    struct sockaddr_in server_addr;

    // 创建 socket
    server->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server->listen_fd < 0) {
        perror("socket");
        return -1;
    }

    // 设置地址重用
    int opt = 1;
    if (setsockopt(server->listen_fd, SOL_SOCKET, SO_REUSEADDR,
                   &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        close(server->listen_fd);
        return -1;
    }

    // 绑定地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server->port);

    if (bind(server->listen_fd, (struct sockaddr*)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("bind");
        close(server->listen_fd);
        return -1;
    }

    // 开始监听
    if (listen(server->listen_fd, BACKLOG) < 0) {
        perror("listen");
        close(server->listen_fd);
        return -1;
    }

    // 设置非阻塞模式
    if (set_nonblocking(server->listen_fd) < 0) {
        perror("set_nonblocking");
        close(server->listen_fd);
        return -1;
    }

    return 0;
}

void tcp_server_stop(tcp_server_t *server) {
    if (!server) return;

    if (server->client_fd >= 0) {
        close(server->client_fd);
        server->client_fd = -1;
    }

    if (server->listen_fd >= 0) {
        close(server->listen_fd);
        server->listen_fd = -1;
    }
}

void tcp_server_process(tcp_server_t *server) {
    if (!server || server->listen_fd < 0) return;

    fd_set read_fds;
    struct timeval tv;
    int max_fd;

    FD_ZERO(&read_fds);
    FD_SET(server->listen_fd, &read_fds);
    max_fd = server->listen_fd;

    if (server->client_fd >= 0) {
        FD_SET(server->client_fd, &read_fds);
        if (server->client_fd > max_fd) {
            max_fd = server->client_fd;
        }
    }

    // 非阻塞 select，超时时间为 0
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    int ret = select(max_fd + 1, &read_fds, NULL, NULL, &tv);
    if (ret < 0) {
        if (errno != EINTR) {
            perror("select");
        }
        return;
    }

    if (ret == 0) {
        return; // 没有事件
    }

    // 检查是否有新连接
    if (FD_ISSET(server->listen_fd, &read_fds)) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        int new_fd = accept(server->listen_fd,
                           (struct sockaddr*)&client_addr, &addr_len);
        if (new_fd >= 0) {
            // 如果已有客户端连接，关闭旧连接
            if (server->client_fd >= 0) {
                printf("[警告] 已有客户端连接，关闭旧连接\n");
                close(server->client_fd);
                if (server->on_disconnected) {
                    server->on_disconnected();
                }
            }

            server->client_fd = new_fd;
            set_nonblocking(server->client_fd);

            // 调用连接回调
            if (server->on_connected) {
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
                server->on_connected(ip, ntohs(client_addr.sin_port));
            }
        }
    }

    // 检查客户端数据
    if (server->client_fd >= 0 && FD_ISSET(server->client_fd, &read_fds)) {
        ssize_t n = recv(server->client_fd, server->recv_buffer,
                        BUFFER_SIZE - 1, 0);

        if (n > 0) {
            server->recv_buffer[n] = '\0';

            // 调用数据接收回调
            if (server->on_data) {
                server->on_data(server->recv_buffer, n);
            }

            // 自动回复确认消息
            const char *ack = "{\"status\":\"ok\",\"message\":\"配置已接收\"}\n";
            send(server->client_fd, ack, strlen(ack), 0);

        } else if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
            // 连接关闭或错误
            close(server->client_fd);
            server->client_fd = -1;

            if (server->on_disconnected) {
                server->on_disconnected();
            }
        }
    }
}

int tcp_server_send(tcp_server_t *server, const char *data, size_t len) {
    if (!server || server->client_fd < 0) {
        return -1;
    }

    return send(server->client_fd, data, len, 0);
}

void tcp_server_destroy(tcp_server_t *server) {
    if (!server) return;

    tcp_server_stop(server);
    free(server);
}
