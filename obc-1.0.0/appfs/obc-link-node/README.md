# OBC-Link Node 节点工具

这是一个用于与 OBC-Link 上位机进行网络通信测试的 C 语言节点工具。

## 功能特性

- ✅ TCP 服务器模式，接收上位机连接
- ✅ 接收并解析 JSON 格式的配置信息
- ✅ 自动回复确认消息
- ✅ 支持自定义监听端口
- ✅ 详细的日志输出和配置打印

## 编译

### 前置要求
- GCC 编译器
- Linux 系统 (Ubuntu 推荐)

### 编译命令

```bash
# 标准编译
make

# 清理编译产物
make clean

# 编译并运行
make run

# 编译调试版本
make debug
```

## 使用方法

### 启动节点工具

```bash
# 使用默认端口 8888
./obc-link-node

# 指定端口
./obc-link-node -p 9999

# 查看帮助
./obc-link-node -h

# 查看版本
./obc-link-node -v
```

### 配置上位机连接

1. 启动 obc-link-node 节点工具
2. 打开 OBC-Link 上位机
3. 在【系统链接信息】页面选择 TCP 连接
4. 输入节点工具所在主机的 IP 地址和端口（默认 8888）
5. 点击连接
6. 在【配置信息】页面填写配置并发送

## 配置格式

节点工具接收的 JSON 配置格式：

```json
{
  "wifiSsid": "MyWiFi",
  "wifiPassword": "password123",
  "deviceName": "OBC-Node-01",
  "deviceId": "device-001",
  "serverUrl": "http://192.168.1.100:8080",
  "autoReconnect": true,
  "heartbeatInterval": 60,
  "remarks": "测试设备"
}
```

## 输出示例

```
====================================
  OBC-Link Node v1.0.0
====================================

[启动] TCP 服务器正在监听端口 8888
[提示] 按 Ctrl+C 退出程序

[连接] 客户端已连接: 192.168.1.100:54321

[接收] 收到数据 (245 字节):
----------------------------------------
{"wifiSsid":"MyWiFi","wifiPassword":"password123",...}
----------------------------------------

[解析] 配置解析成功:
┌─────────────────────────────────────────┐
│          设备配置信息                    │
├─────────────────────────────────────────┤
│ WiFi SSID:         MyWiFi                │
│ WiFi 密码:         password123           │
│ 设备名称:          OBC-Node-01           │
│ 设备 ID:           device-001            │
│ 服务器地址:        http://192.168.1.100:8080 │
│ 自动重连:          是                    │
│ 心跳间隔:          60                    │
│ 备注:              测试设备              │
└─────────────────────────────────────────┘
```

## 测试网络连接

### 本地测试

```bash
# 终端 1: 启动节点工具
./obc-link-node -p 8888

# 终端 2: 使用 netcat 测试连接
nc localhost 8888

# 发送测试 JSON
{"wifiSsid":"TestWiFi","deviceName":"TestDevice"}
```

### 跨机器测试

1. 在目标机器上启动节点工具：
```bash
./obc-link-node -p 8888
```

2. 查看本机 IP 地址：
```bash
ip addr show
# 或
ifconfig
```

3. 在上位机中使用该 IP 地址和端口进行连接

## 文件说明

- `main.c` - 主程序入口
- `tcp_server.c/h` - TCP 服务器实现
- `config_parser.c/h` - JSON 配置解析器
- `Makefile` - 编译脚本
- `README.md` - 本文档

## 技术细节

- **TCP 服务器**: 使用标准 POSIX socket API
- **非阻塞 I/O**: 使用 `select()` 实现事件驱动
- **JSON 解析**: 简单的字符串解析（生产环境建议使用 cJSON 库）
- **信号处理**: 优雅地处理 Ctrl+C 退出

## 后续开发计划

- [ ] 添加串口通信支持
- [ ] 集成完整的 JSON 解析库（如 cJSON）
- [ ] 添加配置持久化
- [ ] 实现心跳机制
- [ ] 添加日志文件输出
- [ ] 支持多客户端连接

## 故障排查

### 端口已被占用
```bash
# 查看端口占用
sudo netstat -tlnp | grep 8888

# 或使用 ss 命令
sudo ss -tlnp | grep 8888

# 更换端口启动
./obc-link-node -p 9999
```

### 无法连接
- 检查防火墙设置
- 确认 IP 地址和端口正确
- 使用 `telnet` 或 `nc` 测试连接性

### 编译错误
- 确保已安装 GCC: `sudo apt install build-essential`
- 检查是否有足够的权限

## 许可证

本项目为测试工具，仅供开发和测试使用。

## 作者

OBC 项目组

## 更新日志

### v1.0.0 (2026-06-30)
- 初始版本
- TCP 服务器功能
- JSON 配置解析
- 基本的日志输出
