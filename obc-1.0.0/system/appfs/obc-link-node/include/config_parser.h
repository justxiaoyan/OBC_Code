/*
 * 配置解析器头文件
 */

#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

// 设备配置结构体
typedef struct {
    char *wifi_ssid;
    char *wifi_password;
    char *device_name;
    char *device_id;
    char *server_url;
    int auto_reconnect;
    int heartbeat_interval;
    char *remarks;
} device_config_t;

/**
 * 解析 JSON 格式的配置
 * @param json_str JSON 字符串
 * @param config 配置结构体指针
 * @return 0 成功，-1 失败
 */
int parse_config_json(const char *json_str, device_config_t *config);

/**
 * 打印配置信息
 * @param config 配置结构体指针
 */
void print_config(const device_config_t *config);

/**
 * 释放配置内存
 * @param config 配置结构体指针
 */
void free_config(device_config_t *config);

#endif // CONFIG_PARSER_H
