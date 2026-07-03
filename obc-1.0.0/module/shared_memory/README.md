# 多核共享内存驱动 (Multicore Shared Memory Driver)

## 概述

这是一个用于嵌入式Linux系统的多核共享内存驱动，主要用于多核处理器之间的高效进程间通信（IPC）。该驱动通过从系统RAM中划分出专用的保留内存区域，提供了一个标准的字符设备接口 `/dev/shared_memory`，使不同核心或进程可以高效地共享数据。

**版权信息**：Copyright (C) 2021-2022 Hikvision Auto Technology Co., Ltd.  
**作者**：wsl <wangshulin@hikvision.com>  
**许可证**：GPL-2.0

---

## 核心特性

### 1. 基础功能
- **保留内存管理**：通过Device Tree的reserved-memory机制管理共享内存区域
- **字符设备接口**：提供 `/dev/shared_memory` 设备节点
- **mmap支持**：用户态程序可通过mmap直接映射共享内存，实现零拷贝访问
- **多区域支持**：可配置多个独立的共享内存区域

### 2. 高级特性

#### 内存重映射（memremap）
- 通过 `pagemap` 模块参数启用
- 为共享内存区域创建 struct page 结构
- 支持TCP/IP零拷贝、hugetlb等高级特性
- 依赖内核配置：`CONFIG_ZONE_DEVICE` 和 `CONFIG_MEMORY_HOTPLUG`

#### 透明大页支持（Transparent Huge Page）
- 通过 `hugepage` 模块参数启用
- 支持页面级别：
  - **PAGE_SIZE**：标准4K页（基础支持）
  - **PMD_SIZE**：2MB大页
  - **PUD_SIZE**：1GB大页（需要 `CONFIG_HAVE_ARCH_TRANSPARENT_HUGEPAGE_PUD`）
- 依赖内核配置：`CONFIG_TRANSPARENT_HUGEPAGE`
- 显著提升大数据量传输的性能

#### 内存对齐控制
- 支持自定义对齐大小（必须是2的幂且PAGE_SIZE的倍数）
- 通过sysfs接口动态调整
- 影响mmap映射的地址对齐

### 3. 系统接口

#### sysfs属性
每个内存区域在 `/sys/class/misc/shared_memory/resourceN/` 下暴露以下属性：
- `align`：对齐大小（可读写）
- `start`：物理起始地址（只读）
- `size`：区域大小（只读）
- `pfn_flags`：页框号标志（只读）

---

## 架构设计

### 数据结构

```c
struct shmem_region {
    unsigned int align;           // 对齐大小
    struct resource res;          // 内存资源描述
    unsigned long pfn_flags;      // 页框标志（PFN_DEV | PFN_MAP）
    struct dev_pagemap pgmap;     // 设备页映射（仅5.10之前）
};

struct shmem_device {
    struct device *dev;           // 设备指针
    int nr_regions;               // 区域数量
    struct shmem_region *regions; // 区域数组
    struct miscdevice miscdev;    // misc设备
    struct dev_pagemap *pgmap;    // 页映射（5.10+）
};
```

### 工作流程

1. **驱动初始化** (`shmem_probe`)
   ```
   解析Device Tree → 请求reserved memory → memremap pages → 注册misc设备
   ```

2. **内存映射** (`shmem_mmap`)
   ```
   查找对应region → 检查VMA对齐 → 配置页保护 → 建立映射
   ```

3. **页面错误处理** (`shmem_vma_fault`)
   - 支持PTE/PMD/PUD三级页表
   - 按需分配和映射页面
   - 支持写时复制（COW）

---

## Device Tree 配置

### 基本配置示例

```dts
reserved-memory {
    #address-cells = <2>;
    #size-cells = <2>;
    ranges;

    /* 定义共享内存区域1：16MB */
    shared_mem_region0: shared-memory@80000000 {
        compatible = "shared-dma-pool";
        reg = <0x0 0x80000000 0x0 0x01000000>;  /* 16MB */
        no-map;  /* 必须设置，表示不映射到内核空间 */
        linux,alignment = <0x200000>;  /* 2MB对齐（支持大页）*/
        linux,memremap;  /* 启用memremap支持 */
    };

    /* 定义共享内存区域2：64MB */
    shared_mem_region1: shared-memory@81000000 {
        compatible = "shared-dma-pool";
        reg = <0x0 0x81000000 0x0 0x04000000>;  /* 64MB */
        no-map;
        linux,alignment = <0x1000>;  /* 4KB对齐 */
    };
};

/* 共享内存设备节点 */
shmem: shared-memory {
    compatible = "linux,multicore-shared-memory";
    memory-region = <&shared_mem_region0>, <&shared_mem_region1>;
    status = "okay";
};
```

### 配置参数说明

| 参数 | 必需 | 说明 |
|------|------|------|
| `compatible` | 是 | 必须为 `"linux,multicore-shared-memory"` |
| `memory-region` | 是 | 引用reserved-memory节点 |
| `no-map` | 是 | 必须在reserved-memory中设置，防止内核映射 |
| `linux,alignment` | 否 | 对齐大小，默认PAGE_SIZE。设置为2MB可支持大页 |
| `linux,memremap` | 否 | 启用memremap支持 |

### 对齐大小建议

| 场景 | 推荐对齐 | 说明 |
|------|----------|------|
| 基础共享内存 | 0x1000 (4KB) | 标准页大小 |
| 高性能传输 | 0x200000 (2MB) | PMD大页 |
| 超大数据块 | 0x40000000 (1GB) | PUD大页（需硬件支持）|

---

## 编译配置

### 内核配置选项

#### 必需配置
```kconfig
CONFIG_BASEAL_BASE=y           # HAT基础抽象层
CONFIG_HAT_SHARED_MEMORY=y     # 启用共享内存驱动
```

#### 可选配置（启用高级特性）
```kconfig
# 启用透明大页支持
CONFIG_TRANSPARENT_HUGEPAGE=y
CONFIG_HAVE_ARCH_TRANSPARENT_HUGEPAGE_PUD=y  # 1GB大页

# 启用memremap支持
CONFIG_MEMORY_HOTPLUG=y
CONFIG_ZONE_DEVICE=y
```

### 编译方法

#### 方法1：作为内核模块编译
```bash
cd /path/to/hatos-6.0.0/modules/drivers/shared_memory
make
```

#### 方法2：集成到内核
在内核配置菜单中启用：
```
Device Drivers → HAT-BSP drivers → shared memory driver
```

---

## 使用方法

### 1. 加载驱动

#### 基础模式
```bash
insmod multicore-shared-memory.ko
```

#### 启用内存重映射
```bash
insmod multicore-shared-memory.ko pagemap=1
```

#### 启用大页支持
```bash
insmod multicore-shared-memory.ko pagemap=1 hugepage=1
```

### 2. 查看设备信息

```bash
# 查看设备节点
ls -l /dev/shared_memory

# 查看内存区域信息
cat /sys/class/misc/shared_memory/resource0/start   # 起始地址
cat /sys/class/misc/shared_memory/resource0/size    # 大小
cat /sys/class/misc/shared_memory/resource0/align   # 对齐

# 查看所有区域
ls /sys/class/misc/shared_memory/
```

### 3. 用户态编程示例

#### 基础mmap示例

```c
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>

int main() {
    int fd;
    void *addr;
    size_t size = 16 * 1024 * 1024;  // 16MB
    off_t offset = 0x80000000;        // 物理地址偏移
    
    /* 打开共享内存设备 */
    fd = open("/dev/shared_memory", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open");
        return -1;
    }
    
    /* 映射共享内存 */
    addr = mmap(NULL, size, PROT_READ | PROT_WRITE, 
                MAP_SHARED, fd, offset);
    if (addr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }
    
    printf("Mapped at: %p\n", addr);
    
    /* 写入数据 */
    uint32_t *data = (uint32_t *)addr;
    data[0] = 0xDEADBEEF;
    
    /* 读取数据 */
    printf("Data: 0x%X\n", data[0]);
    
    /* 清理 */
    munmap(addr, size);
    close(fd);
    
    return 0;
}
```

#### 多进程通信示例

**进程A（写入者）**：
```c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>

#define SHMEM_SIZE (1024 * 1024)  // 1MB
#define SHMEM_OFFSET 0x80000000

int main() {
    int fd = open("/dev/shared_memory", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open");
        return -1;
    }
    
    char *addr = mmap(NULL, SHMEM_SIZE, PROT_READ | PROT_WRITE,
                      MAP_SHARED, fd, SHMEM_OFFSET);
    if (addr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }
    
    /* 写入消息 */
    const char *msg = "Hello from Process A!";
    strcpy(addr, msg);
    printf("Writer: Wrote '%s'\n", msg);
    
    /* 保持映射，等待读取 */
    printf("Press Enter to exit...\n");
    getchar();
    
    munmap(addr, SHMEM_SIZE);
    close(fd);
    return 0;
}
```

**进程B（读取者）**：
```c
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#define SHMEM_SIZE (1024 * 1024)
#define SHMEM_OFFSET 0x80000000

int main() {
    int fd = open("/dev/shared_memory", O_RDONLY);
    if (fd < 0) {
        perror("open");
        return -1;
    }
    
    char *addr = mmap(NULL, SHMEM_SIZE, PROT_READ,
                      MAP_SHARED, fd, SHMEM_OFFSET);
    if (addr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return -1;
    }
    
    /* 读取消息 */
    printf("Reader: Read '%s'\n", addr);
    
    munmap(addr, SHMEM_SIZE);
    close(fd);
    return 0;
}
```

#### 大页优化示例

```c
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>

#define HUGE_PAGE_SIZE (2 * 1024 * 1024)  // 2MB
#define SHMEM_OFFSET 0x80000000

int main() {
    int fd = open("/dev/shared_memory", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open");
        return -1;
    }
    
    /* 使用MAP_HUGETLB标志请求大页 */
    void *addr = mmap(NULL, HUGE_PAGE_SIZE, 
                      PROT_READ | PROT_WRITE,
                      MAP_SHARED | MAP_HUGETLB, 
                      fd, SHMEM_OFFSET);
    if (addr == MAP_FAILED) {
        perror("mmap with huge page");
        close(fd);
        return -1;
    }
    
    printf("Huge page mapped at: %p\n", addr);
    
    /* 使用共享内存... */
    
    munmap(addr, HUGE_PAGE_SIZE);
    close(fd);
    return 0;
}
```

#### 动态调整对齐大小

```bash
# 查看当前对齐
cat /sys/class/misc/shared_memory/resource0/align

# 修改为2MB对齐（支持大页）
echo 0x200000 > /sys/class/misc/shared_memory/resource0/align

# 验证
cat /sys/class/misc/shared_memory/resource0/align
```

**注意**：对齐大小只能在内存区域未使用前修改。

---

## 性能优化建议

### 1. 启用大页支持
- **场景**：传输大块数据（视频流、图像帧等）
- **配置**：
  ```bash
  insmod multicore-shared-memory.ko pagemap=1 hugepage=1
  ```
- **效果**：减少TLB miss，提升20-40%性能

### 2. 使用适当的对齐
| 数据大小 | 推荐对齐 | 原因 |
|----------|----------|------|
| < 1MB | 4KB | 减少内存浪费 |
| 1-100MB | 2MB | 平衡性能与内存 |
| > 100MB | 2MB或1GB | 最大化TLB效率 |

### 3. 使用O_SYNC标志
- 对于需要强一致性的场景，打开设备时使用 `O_SYNC`
- 驱动会自动配置为 write-combining 模式

### 4. 内存屏障
在多核通信时，注意使用适当的内存屏障：
```c
#include <linux/compiler.h>

/* 写入数据后 */
barrier();  // 编译器屏障
__sync_synchronize();  // 硬件内存屏障
```

---

## 故障排查

### 常见问题

#### 1. 设备节点不存在
```bash
# 问题
ls: cannot access '/dev/shared_memory': No such file or directory

# 检查
dmesg | grep -i shared
lsmod | grep shared

# 解决
insmod multicore-shared-memory.ko
```

#### 2. mmap失败（EPERM）
```
# 原因：物理地址不在任何reserved memory区域内
# 解决：检查Device Tree配置和mmap的offset参数

# 查看可用区域
cat /sys/class/misc/shared_memory/resource*/start
cat /sys/class/misc/shared_memory/resource*/size
```

#### 3. 对齐错误（EINVAL）
```
# 原因：VMA范围未按region的对齐要求对齐
# 解决：确保mmap的大小是对齐大小的整数倍

# 查看对齐要求
cat /sys/class/misc/shared_memory/resource0/align
```

#### 4. 大页不生效
```bash
# 检查内核配置
zcat /proc/config.gz | grep TRANSPARENT_HUGEPAGE

# 检查模块参数
cat /sys/module/multicore_shared_memory/parameters/hugepage
cat /sys/module/multicore_shared_memory/parameters/pagemap

# 查看驱动警告
dmesg | grep shared_memory
```

### 调试技巧

#### 启用内核调试信息
```bash
# 动态调试
echo 'module multicore_shared_memory +p' > /sys/kernel/debug/dynamic_debug/control

# 查看内核日志
dmesg -w
```

#### 检查内存映射
```bash
# 查看进程的内存映射
cat /proc/<pid>/maps | grep shared_memory

# 查看页表统计
cat /proc/<pid>/smaps | grep -A 20 shared_memory
```

---

## 应用场景

### 1. 多核处理器IPC
- **场景**：ARM大小核、多DSP核心通信
- **优势**：零拷贝、低延迟

### 2. 视频流处理
- **场景**：摄像头数据共享给多个处理进程
- **优势**：大页支持、高带宽

### 3. 机器学习推理
- **场景**：CPU与加速器（NPU/GPU）数据交换
- **优势**：内存重映射支持、零拷贝

### 4. 实时操作系统通信
- **场景**：Linux与RTOS核心数据交换
- **优势**：预留内存、确定性延迟

---

## 技术细节

### 内存映射模式

#### 模式1：标准模式（不启用pagemap）
```
用户空间 --mmap--> VMA --remap_pfn_range--> 物理内存
```
- 直接建立页表映射
- 无struct page结构
- 不支持高级特性

#### 模式2：pagemap模式
```
用户空间 --mmap--> VMA --vmf_insert_mixed--> dev_pagemap --> 物理内存
```
- 创建struct page
- 支持get_user_pages、TCP零拷贝等
- 支持透明大页

### 页面错误处理流程

```
page fault
    |
    v
shmem_vma_huge_fault
    |
    +-- PE_SIZE_PTE  --> __shmem_pte_fault  (4KB页)
    +-- PE_SIZE_PMD  --> __shmem_pmd_fault  (2MB页)
    +-- PE_SIZE_PUD  --> __shmem_pud_fault  (1GB页)
```

### 内核版本差异

| 特性 | Linux < 5.10 | Linux >= 5.10 |
|------|--------------|---------------|
| dev_pagemap | 每region一个 | 全局统一管理 |
| resource请求 | devm_request_resource | 自动管理 |
| memremap类型 | MEMORY_DEVICE_NORMAL_DMA | MEMORY_DEVICE_GENERIC |

---

## 限制与注意事项

### 限制
1. **reserved memory必须设置no-map属性**
2. **对齐大小必须是2的幂且PAGE_SIZE的倍数**
3. **大页支持需要memremap启用**
4. **IOCTL功能当前未实现**（返回-ENOTSUPP）

### 注意事项
1. **缓存一致性**：多核访问时需手动管理缓存
2. **同步机制**：需要上层实现锁或信号量
3. **内存泄漏**：确保munmap与mmap成对调用
4. **权限控制**：设备节点默认权限可能需要调整

---

## 版本历史

- **v1.01.000**：初始版本
  - 支持多区域共享内存
  - 支持透明大页
  - 支持内存重映射
  - 兼容Linux 5.10前后版本

---

## 参考资料

### 相关内核文档
- `Documentation/devicetree/bindings/reserved-memory/reserved-memory.txt`
- `Documentation/vm/hugetlbpage.rst`
- `Documentation/driver-api/device-io.rst`

### 相关内核配置
- `CONFIG_TRANSPARENT_HUGEPAGE`
- `CONFIG_ZONE_DEVICE`
- `CONFIG_MEMORY_HOTPLUG`

### 相关系统调用
- `mmap(2)` - 内存映射
- `munmap(2)` - 取消映射
- `mlock(2)` - 锁定内存页

---

## 联系方式

如有问题或建议，请联系：
- **作者**：wsl <wangshulin@hikvision.com>
- **组织**：Hikvision Auto Technology Co., Ltd.

---

## 附录：完整示例项目

### Makefile
```makefile
CC = gcc
CFLAGS = -Wall -O2

all: writer reader test_hugepage

writer: writer.c
	$(CC) $(CFLAGS) -o writer writer.c

reader: reader.c
	$(CC) $(CFLAGS) -o reader reader.c

test_hugepage: test_hugepage.c
	$(CC) $(CFLAGS) -o test_hugepage test_hugepage.c

clean:
	rm -f writer reader test_hugepage
```

### 测试脚本
```bash
#!/bin/bash
# test_shared_memory.sh

echo "=== Shared Memory Driver Test ==="

# 1. 检查设备
if [ ! -c /dev/shared_memory ]; then
    echo "Error: /dev/shared_memory not found"
    exit 1
fi

# 2. 显示配置
echo "Memory regions:"
for dir in /sys/class/misc/shared_memory/resource*; do
    [ -d "$dir" ] || continue
    echo "  $(basename $dir):"
    echo "    start: $(cat $dir/start)"
    echo "    size:  $(cat $dir/size)"
    echo "    align: $(cat $dir/align)"
done

# 3. 运行测试
echo "Running writer in background..."
./writer &
WRITER_PID=$!

sleep 1

echo "Running reader..."
./reader

kill $WRITER_PID 2>/dev/null

echo "Test completed!"
```

---

**文档版本**：1.0  
**最后更新**：2024
