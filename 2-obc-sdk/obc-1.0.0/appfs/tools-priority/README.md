# Priority Tools - 线程优先级和调度策略工具集

## 概述

这是一个用于查看和演示Linux系统线程优先级和调度策略的工具集，包含两个工具：

1. **priority_dump** - 线程信息查看工具
2. **priority_demo** - 调度策略演示程序

## 工具说明

### priority_dump - 线程信息查看工具

查看系统中所有线程的详细调度信息，包括：
- **PID** - 线程ID
- **线程名** - 线程名称
- **CPU号** - 当前运行的CPU核心
- **优先级** - 调度优先级（实时线程1-99，普通线程0）
- **NICE值** - 普通线程的优先级调整值（-20到19）
- **调度策略** - SCHED_NORMAL、SCHED_FIFO、SCHED_RR、SCHED_BATCH等
- **CPU占用** - 相对CPU使用时间占比

**使用场景：**
- 调试实时应用程序
- 分析线程调度问题
- 监控系统线程状态
- 验证优先级设置

### priority_demo - 调度策略演示程序

创建三个不同调度策略的线程用于演示和测试：
- **RR-Thread** - 实时轮转调度（SCHED_RR），优先级50
- **FIFO-Thread** - 实时先进先出调度（SCHED_FIFO），优先级30
- **NORMAL-Thread** - 普通调度（SCHED_NORMAL），优先级0

**使用场景：**
- 学习Linux调度策略
- 测试实时调度功能
- 验证优先级设置是否生效
- 演示不同调度策略的行为

## 快速开始

### 编译

```bash
# 编译priority_dump
make

# 编译priority_demo
make demo

# 编译并优化（推荐）
make strip strip-demo

# 清理编译产物
make clean
```

### 运行

```bash
# 1. 运行演示程序（需要root权限设置实时优先级）
sudo ./demo/priority_demo

# 2. 在另一个终端查看线程信息
./priority_dump | grep priority_demo
```

### 输出示例

**priority_demo输出：**
```
====================================
Thread Priority Demonstration
====================================
[RR-Thread] Created successfully - TID: 1234
[FIFO-Thread] Created successfully - TID: 1235
[NORMAL-Thread] Created successfully - TID: 1236
====================================

Use 'priority_dump' to view thread details.
Press Ctrl+C to exit.
```

**priority_dump输出：**
```
PID      THREAD_NAME                         CPU    PRIORITY NICE   POLICY          CPU%
-------- ----------------------------------- ------ -------- ------ --------------- --------
746      priority_demo                       2      20       0      SCHED_NORMAL      0.02%
747      RR-Thread                           1      50       0      SCHED_RR          0.01%
748      FIFO-Thread                         0      30       0      SCHED_FIFO        0.01%
749      NORMAL-Thread                       3      20       0      SCHED_NORMAL      0.01%
```

## 编译方法

### 前置条件

- 交叉编译工具链（默认路径在Makefile中配置）
- ARM64目标平台（AM62x）

### Makefile目标

| 命令 | 说明 |
|------|------|
| `make` | 编译priority_dump |
| `make demo` | 编译priority_demo |
| `make strip` | 编译并strip priority_dump |
| `make strip-demo` | 编译并strip priority_demo |
| `make clean` | 清理所有编译产物 |
| `make clean-demo` | 只清理demo |
| `make install DESTDIR=/path` | 安装priority_dump到指定路径 |
| `make install-demo DESTDIR=/path` | 安装priority_demo到指定路径 |
| `make install-all DESTDIR=/path` | 安装所有工具 |
| `make help` | 显示帮助信息 |

### 编译示例

```bash
# 编译所有工具并strip
make strip strip-demo

# 安装到根文件系统
make install-all DESTDIR=/mnt/rootfs

# 或者手动拷贝到NFS目录
cp priority_dump ~/nfs/
cp demo/priority_demo ~/nfs/
```

## 调度策略说明

### SCHED_NORMAL (0)
- **普通调度策略**，使用CFS（完全公平调度器）
- 适用于大多数应用程序
- 优先级通过NICE值调整（-20到19，值越小优先级越高）

### SCHED_FIFO (1)
- **先进先出实时调度**
- 优先级1-99（数值越大优先级越高）
- 线程会一直运行直到主动让出CPU或被更高优先级线程抢占
- 需要root权限

### SCHED_RR (2)
- **轮转实时调度**
- 优先级1-99（数值越大优先级越高）
- 同优先级的线程轮流执行（时间片轮转）
- 需要root权限

### SCHED_BATCH (3)
- **批处理调度**，适用于CPU密集型任务
- 不适合交互式应用

### SCHED_IDLE (5)
- **空闲调度**，最低优先级
- 只在系统空闲时运行

### SCHED_DEADLINE (6)
- **截止期限调度**，用于实时任务
- 需要精确的时间约束

## 技术特性

- ✅ **静态编译** - 无动态库依赖，可独立运行
- ✅ **ARM64架构** - 针对AM62x平台优化
- ✅ **低CPU占用** - demo线程适度休眠，避免空转
- ✅ **完整的调度策略支持** - 支持所有Linux调度策略
- ✅ **实时优先级** - 支持1-99级实时优先级设置

## 注意事项

1. **root权限**：设置实时调度策略（SCHED_FIFO/SCHED_RR）需要root权限
2. **CPU占用**：demo程序已优化，每10ms休眠一次，避免满载运行
3. **线程数量**：priority_dump会显示系统所有线程，包括内核线程
4. **CPU百分比**：显示的是累计运行时间占比，不是实时CPU使用率

## 文件结构

```
priority_dump/
├── Makefile              # 主Makefile
├── priority_dump.c       # priority_dump源码
├── README.md            # 本文档
├── test.sh              # 测试脚本
└── demo/                # 演示程序目录
    ├── Makefile         # demo的Makefile
    ├── priority_demo.c  # priority_demo源码
    └── README.md        # demo详细文档
```

## 使用场景示例

### 场景1：调试实时应用

```bash
# 查看系统所有实时线程
./priority_dump | grep -E "SCHED_(FIFO|RR)"

# 按CPU使用率排序查看
./priority_dump -s | head -20
```

### 场景2：验证优先级设置

```bash
# 运行自己的实时程序
sudo ./my_realtime_app &

# 验证优先级是否正确设置
./priority_dump | grep my_realtime_app
```

### 场景3：学习调度策略

```bash
# 运行演示程序
sudo ./demo/priority_demo &

# 观察不同调度策略的线程
./priority_dump | grep priority_demo

# 使用watch实时监控
watch -n 1 './priority_dump | grep priority_demo'
```

## 许可证

与AM62X-SDK项目保持一致

## 作者

AM62X-SDK开发团队
