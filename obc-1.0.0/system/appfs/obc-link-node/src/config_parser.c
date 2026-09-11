/*
 * 配置解析器实现（简单的 JSON 解析）
 */

#include "config_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// 简单的 JSON 字符串提取函数
static char* extract_string_value(const char *json, const char *key) {
    char search_key[256];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);

    const char *key_pos = strstr(json, search_key);
    if (!key_pos) return NULL;

    // 找到冒号
    const char *colon = strchr(key_pos, ':');
    if (!colon) return NULL;

    // 跳过空白字符
    colon++;
    while (*colon && isspace(*colon)) colon++;

    // 如果是字符串（以引号开始）
    if (*colon == '"') {
        colon++;
        const char *end = strchr(colon, '"');
        if (!end) return NULL;

        size_t len = end - colon;
        char *value = (char*)malloc(len + 1);
        if (!value) return NULL;

        strncpy(value, colon, len);
        value[len] = '\0';
        return value;
    }

    return NULL;
}

// 提取整数值
static int extract_int_value(const char *json, const char *key, int default_value) {
    char search_key[256];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);

    const char *key_pos = strstr(json, search_key);
    if (!key_pos) return default_value;

    const char *colon = strchr(key_pos, ':');
    if (!colon) return default_value;

    colon++;
    while (*colon && isspace(*colon)) colon++;

    return atoi(colon);
}

// 提取布尔值
static int extract_bool_value(const char *json, const char *key, int default_value) {
    char search_key[256];
    snprintf(search_key, sizeof(search_key), "\"%s\"", key);

    const char *key_pos = strstr(json, search_key);
    if (!key_pos) return default_value;

    const char *colon = strchr(key_pos, ':');
    if (!colon) return default_value;

    colon++;
    while (*colon && isspace(*colon)) colon++;

    if (strncmp(colon, "true", 4) == 0) return 1;
    if (strncmp(colon, "false", 5) == 0) return 0;

    return default_value;
}

int parse_config_json(const char *json_str, device_config_t *config) {
    if (!json_str || !config) return -1;

    memset(config, 0, sizeof(device_config_t));

    // 解析各个字段
    config->wifi_ssid = extract_string_value(json_str, "wifiSsid");
    config->wifi_password = extract_string_value(json_str, "wifiPassword");
    config->device_name = extract_string_value(json_str, "deviceName");
    config->device_id = extract_string_value(json_str, "deviceId");
    config->server_url = extract_string_value(json_str, "serverUrl");
    config->remarks = extract_string_value(json_str, "remarks");

    config->auto_reconnect = extract_bool_value(json_str, "autoReconnect", 1);
    config->heartbeat_interval = extract_int_value(json_str, "heartbeatInterval", 60);

    // 检查是否至少解析到一个字段
    if (!config->wifi_ssid && !config->device_name && !config->device_id) {
        free_config(config);
        return -1;
    }

    return 0;
}

void print_config(const device_config_t *config) {
    if (!config) return;

    printf("┌─────────────────────────────────────────┐\n");
    printf("│          设备配置信息                    │\n");
    printf("├─────────────────────────────────────────┤\n");

    if (config->wifi_ssid) {
        printf("│ WiFi SSID:         %-20s │\n", config->wifi_ssid);
    }
    if (config->wifi_password) {
        printf("│ WiFi 密码:         %-20s │\n", config->wifi_password);
    }
    if (config->device_name) {
        printf("│ 设备名称:          %-20s │\n", config->device_name);
    }
    if (config->device_id) {
        printf("│ 设备 ID:           %-20s │\n", config->device_id);
    }
    if (config->server_url) {
        printf("│ 服务器地址:        %-20s │\n", config->server_url);
    }

    printf("│ 自动重连:          %-20s │\n",
           config->auto_reconnect ? "是" : "否");
    printf("│ 心跳间隔:          %-20d │\n",
           config->heartbeat_interval);

    if (config->remarks) {
        printf("│ 备注:              %-20s │\n", config->remarks);
    }

    printf("└─────────────────────────────────────────┘\n");
}

void free_config(device_config_t *config) {
    if (!config) return;

    free(config->wifi_ssid);
    free(config->wifi_password);
    free(config->device_name);
    free(config->device_id);
    free(config->server_url);
    free(config->remarks);

    memset(config, 0, sizeof(device_config_t));
}
