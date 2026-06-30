/*
 * TCP 服务器头文件
 */

#ifndef TCP_SERVER_H
#define TCP_SERVER_H

#include <stddef.h>

// TCP 服务器结构体（不透明）
typedef struct tcp_server tcp_server_t;

// 回调函数类型定义
typedef void (*data_callback_t)(const char *data, size_t len);
typedef void (*connected_callback_t)(const char *client_ip, int client_port);
typedef void (*disconnected_callback_t)(void);

/**
 * 创建 TCP 服务器
 * @param port 监听端口
 * @return 服务器实例，失败返回 NULL
 */
tcp_server_t* tcp_server_create(int port);

/**
 * 设置回调函数
 * @param server 服务器实例
 * @param on_data 数据接收回调
 * @param on_connected 客户端连接回调
 * @param on_disconnected 客户端断开回调
 */
void tcp_server_set_callbacks(tcp_server_t *server,
                              data_callback_t on_data,
                              connected_callback_t on_connected,
                              disconnected_callback_t on_disconnected);

/**
 * 启动服务器
 * @param server 服务器实例
 * @return 0 成功，-1 失败
 */
int tcp_server_start(tcp_server_t *server);

/**
 * 停止服务器
 * @param server 服务器实例
 */
void tcp_server_stop(tcp_server_t *server);

/**
 * 处理服务器事件（非阻塞）
 * @param server 服务器实例
 */
void tcp_server_process(tcp_server_t *server);

/**
 * 向客户端发送数据
 * @param server 服务器实例
 * @param data 数据缓冲区
 * @param len 数据长度
 * @return 发送的字节数，失败返回 -1
 */
int tcp_server_send(tcp_server_t *server, const char *data, size_t len);

/**
 * 销毁服务器
 * @param server 服务器实例
 */
void tcp_server_destroy(tcp_server_t *server);

#endif // TCP_SERVER_H
