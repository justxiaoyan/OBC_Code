# Shared Memory 驱动模块集成说明

## 概述

shared_memory 驱动模块已成功集成到 OBC 编译框架中，支持通过 menuconfig 配置，并可根据不同平台动态匹配内核路径进行编译。

## 功能特性

- ✅ 通过 menuconfig 配置是否编译驱动
- ✅ 支持多平台动态内核路径（imx6ull, rv1106, rk3562, platdemo）
- ✅ 独立实现，不依赖外部驱动框架
- ✅ 标准的 Linux 内核模块接口
- ✅ 支持 IOCTL 控制接口
- ✅ 自动安装到 output/modules 目录

## 目录结构

```
obc-1.0.0/
├── module/
│   ├── Kconfig                          # 模块配置菜单
│   ├── module.mk                        # 模块编译配置
│   └── shared_memory/
│       ├── Config.in                    # 兼容配置文件
│       ├── Makefile                     # 模块 Makefile
│       ├── multicore-shared-memory.c    # 驱动源码
│       ├── shared_memory_ioctl.h        # IOCTL 定义
│       └── README.md                    # 驱动说明文档
└── output/
    └── modules/                         # 编译输出目录
        └── multicore-shared-memory.ko   # 编译生成的驱动模块
```

## 使用方法

### 1. 选择平台

```bash
cd /home/ayan-server/HDD-B/justxiaoyan-github/OBC_Code/obc-1.0.0
make platform
# 选择对应的平台，例如：
#   [1] imx6ull
#   [2] rv1106
#   [3] rk3562
#   [4] platdemo
```

### 2. 配置模块（可选）

通过 menuconfig 启用或禁用 shared_memory 驱动：

```bash
make menuconfig
```

导航到：`Kernel Modules` -> `[*] Enable shared_memory driver (/dev/shared_memory)`

或者直接编辑 `.config` 文件，添加：
```
CONFIG_MODULE_SHARED_MEMORY=y
```

### 3. 编译模块

```bash
make module
```

输出示例：
```
Building kernel modules...
  Building module: shared_memory
Building multicore-shared-memory module...
  KDIR: .../obc_sdk/imx6ull_sdk_source/kernel
  ARCH: arm
  CROSS_COMPILE: arm-buildroot-linux-gnueabihf-
  CC [M]  .../multicore-shared-memory.o
  MODPOST .../Module.symvers
  LD [M]  .../multicore-shared-memory.ko
```

### 4. 安装模块

```bash
make module_install
```

模块将被安装到：`output/modules/multicore-shared-memory.ko`

### 5. 清理模块

```bash
make module_clean
```

### 6. 保存配置

将当前配置保存到平台 defconfig：

```bash
make savedefconfig
```

## 平台内核路径映射

驱动编译时会根据选择的平台自动匹配对应的内核路径：

| 平台 | SDK 名称 | 内核路径 |
|------|----------|----------|
| imx6ull | imx6ull_sdk_source | obc_sdk/imx6ull_sdk_source/kernel |
| rv1106 | rv1106_sdk_source | obc_sdk/rv1106_sdk_source/kernel |
| rk3562 | rk3562_sdk_source | obc_sdk/rk3562_sdk_source/kernel |
| platdemo | platdemo_sdk_source | obc_sdk/platdemo_sdk_source/kernel |

## 驱动功能

### 设备节点

驱动成功加载后，会创建设备节点：`/dev/shared_memory`

### IOCTL 接口

```c
#include "shared_memory_ioctl.h"

// 获取内存区域数量
int count;
ioctl(fd, SHMEM_IOC_GET_REGION_COUNT, &count);

// 获取内存区域信息
struct shmem_region_info info;
info.index = 0;  // 区域索引
ioctl(fd, SHMEM_IOC_GET_REGION_INFO, &info);
// info.phys_addr: 物理地址
// info.size: 内存大小
// info.align: 对齐要求
```

### mmap 映射

```c
int fd = open("/dev/shared_memory", O_RDWR);
void *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, 
                  MAP_SHARED, fd, offset);
// 使用共享内存...
munmap(addr, size);
close(fd);
```

## 设备树配置

驱动需要在设备树中配置 reserved memory 区域：

```dts
reserved-memory {
    #address-cells = <2>;
    #size-cells = <2>;
    ranges;

    shared_memory: shared-mem@80000000 {
        compatible = "shared-memory";
        reg = <0x0 0x80000000 0x0 0x1000000>;  // 16MB
        no-map;
    };
};

shared_mem_device {
    compatible = "obc,shared-memory";
    memory-region = <&shared_memory>;
    status = "okay";
};
```

## 完整编译流程

```bash
# 1. 选择平台
make platform

# 2. 配置（可选）
make menuconfig  # 或直接编辑 .config

# 3. 编译所有组件（包括模块）
make all

# 4. 安装所有组件
make install

# 5. 查看编译结果
ls -lh output/modules/
ls -lh output/tmp/
ls -lh output/image/

# 6. 保存配置
make savedefconfig
```

## 验证

编译成功后，检查模块文件：

```bash
$ file output/modules/multicore-shared-memory.ko
output/modules/multicore-shared-memory.ko: ELF 32-bit LSB relocatable, ARM, EABI5 version 1 (SYSV)

$ ls -lh output/modules/
total 12K
-rw-rw-r-- 1 user user 12K Jul  3 10:15 multicore-shared-memory.ko
```

## 添加新模块

如需添加其他驱动模块，参考以下步骤：

1. 在 `module/` 下创建新模块目录
2. 在 `module/Kconfig` 中添加配置选项
3. 在 `module/module.mk` 中添加模块到 `MODULES` 列表
4. 创建模块的 Makefile（参考 shared_memory/Makefile）

## 故障排除

### 问题：编译时找不到内核目录

**解决方案**：确保已经编译过内核，或者运行：
```bash
cd obc_sdk/<platform>_sdk_source/kernel
make scripts ARCH=arm CROSS_COMPILE=<toolchain>
make modules_prepare ARCH=arm CROSS_COMPILE=<toolchain>
```

### 问题：模块配置不生效

**解决方案**：
1. 确认 `.config` 中有 `CONFIG_MODULE_SHARED_MEMORY=y`
2. 运行 `make savedefconfig` 保存配置
3. 清理并重新编译：`make module_clean && make module`

### 问题：交叉编译工具链找不到

**解决方案**：
1. 检查 `.config` 中的 `CONFIG_OBC_SDK_COMP` 配置
2. 确保交叉编译工具链在 PATH 中
3. 或在编译时指定：`CROSS_COMPILE=<path-to-toolchain>`

## 技术细节

### 驱动特性

- 基于 platform_driver 框架
- 使用 miscdevice 创建字符设备
- 支持 device tree 配置
- 支持多个内存区域
- 支持内存映射（mmap）
- 提供 IOCTL 查询接口

### 编译系统集成

- 与 appfs 同级的配置方式
- 通过 Kconfig/menuconfig 管理
- 支持增量编译
- 自动依赖检查
- 平台无关的 Makefile 设计

## 相关文档

- [OBC编译框架说明](../../README.md)
- [内核模块开发指南](https://www.kernel.org/doc/html/latest/kbuild/modules.html)
- [设备树规范](https://www.devicetree.org/)

## 更新日志

### 2026-07-03
- ✅ 完成 shared_memory 驱动集成
- ✅ 添加 menuconfig 配置支持
- ✅ 实现多平台动态内核路径
- ✅ 创建独立驱动实现（不依赖外部框架）
- ✅ 添加 make module/module_install/module_clean 命令
- ✅ 更新帮助文档
