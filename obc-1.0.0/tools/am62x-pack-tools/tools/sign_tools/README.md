# EMS 签名工具

## 概述

EMS 签名工具用于对二进制文件进行封装和校验，主要应用于嵌入式系统固件的打包与验证流程。工具在原始二进制数据前附加一个 512 字节的签名头，包含文件元信息和 CRC16 校验码，确保文件传输和存储的完整性。

本工具集包含两个独立程序：
- `ems_sign`: 将原始二进制文件打包为带签名头的文件
- `unsign_demo`: 解包已签名文件并验证数据完整性

## 签名头结构

签名头固定为 512 字节，采用联合体实现对齐。数据结构如下：

```
偏移量    长度    字段名            说明
------    ----    --------          ----
0x000     6       magic             魔数标识，固定为 "EMSFS"
0x006     4       file_size         原始文件大小（字节）
0x00A     2       crc16             CRC16 校验值
0x00C     2       head_write_flag   头部写入标志（1=写入升级分区，0=不写入）
0x00E     64      pack_file         输出文件名
0x04E     64      file_name         原始文件名
0x08E     ~       -                 填充至 512 字节
```

### 字段说明

**magic (6 bytes)**  
魔数标识符，用于快速识别文件格式。固定值为 ASCII 字符串 "EMSFS"，以 NULL 结尾。

**file_size (4 bytes)**  
原始数据的实际大小，单位为字节，采用小端序存储。不包含 512 字节头部。

**crc16 (2 bytes)**  
对原始数据计算的 CRC16 校验值，使用标准多项式 0x8005，初始值 0xFFFF，无反转。校验范围为签名头之后的全部数据。

**head_write_flag (2 bytes)**  
头部写入标志位，用于指示升级流程中是否需要将签名头写入升级分区。
- 值为 1：需要将 512 字节签名头写入升级分区
- 值为 0：不写入签名头，仅写入原始数据

**pack_file (64 bytes)**  
记录打包时指定的输出文件路径，字符串以 NULL 结尾。

**file_name (64 bytes)**  
提取自输入文件路径的文件名部分，不含目录路径，以 NULL 结尾。

## 构建

### 编译环境

- GCC 编译器（支持 C99 及以上标准）
- GNU Make 3.8+

### 编译命令

在 `sign_tools` 目录下执行：

```bash
make
```

编译完成后，可执行文件输出至 `output/` 目录：
- `output/ems_sign`
- `output/unsign_demo`

### 清理

```bash
make clean
```

## 使用方法

### 1. 打包文件 (ems_sign)

将原始二进制文件打包为带签名头的文件。支持通过 `-h` 参数设置头部写入标志。

**命令格式：**
```bash
ems_sign [-h] <input_file> <output_file>
```

**参数说明：**
- `-h`: 可选参数，设置 head_write_flag 为 1，表示需要将签名头写入升级分区
- `input_file`: 待打包的原始二进制文件路径
- `output_file`: 生成的带签名头文件路径

**示例 1 - 普通打包（不写入头部）：**
```bash
./output/ems_sign u-boot.bin u-boot-signed.bin
```

**输出信息：**
```
Calculated CRC16: 0xABCD
Packing completed successfully.
  Input file:  u-boot.bin (524288 bytes)
  Output file: u-boot-signed.bin
  CRC16:       0xABCD
  Head Write:  0 (NO)
```

**示例 2 - 带头部写入标志的打包：**
```bash
./output/ems_sign -h u-boot.bin u-boot-signed.bin
```

**输出信息：**
```
Calculated CRC16: 0xABCD
Packing completed successfully.
  Input file:  u-boot.bin (524288 bytes)
  Output file: u-boot-signed.bin
  CRC16:       0xABCD
  Head Write:  1 (YES)
```

**打包后文件结构：**
```
[512-byte header] + [原始文件内容]
```

### 2. 解包与验证 (unsign_demo)

从签名文件中提取原始数据并验证完整性。

**命令格式（仅查看信息）：**
```bash
unsign_demo <signed_file>
```

**命令格式（解包数据）：**
```bash
unsign_demo <signed_file> <output_file>
```

**参数说明：**
- `signed_file`: 带签名头的文件
- `output_file`: 提取后的原始文件保存路径（可选）

**示例 1 - 查看签名信息：**
```bash
./output/unsign_demo u-boot-signed.bin
```

**输出：**
```
========== EMS Package Header ==========
Magic:       EMSFS
Pack File:   u-boot-signed.bin
File Name:   u-boot.bin
File Size:   524288 bytes
CRC16:       0xABCD
Head Write:  0 (NO)
========================================
```

**示例 2 - 解包并验证：**
```bash
./output/unsign_demo u-boot-signed.bin u-boot-extracted.bin
```

**输出：**
```
========== EMS Package Header ==========
Magic:       EMSFS
Pack File:   u-boot-signed.bin
File Name:   u-boot.bin
File Size:   524288 bytes
CRC16:       0xABCD
Head Write:  0 (NO)
========================================

CRC16 Verification:
  Stored CRC:     0xABCD
  Calculated CRC: 0xABCD
  Status:         PASS

Unpacking completed successfully.
  Output file: u-boot-extracted.bin
```

### 错误处理

**魔数校验失败：**
```
Invalid magic number: expected 'EMSFS', got 'XXXXX'
```
原因：文件不是有效的 EMS 签名文件或已损坏。

**CRC16 校验失败：**
```
CRC16 verification FAILED! Data may be corrupted.
```
原因：文件内容在传输或存储过程中被篡改或损坏。

**文件大小异常：**
```
Invalid file size: 0 bytes
```
原因：签名头中记录的文件大小为 0 或超过限制（最大 256MB）。

## 技术细节

### CRC16 算法

- **算法类型**: 查表法
- **多项式**: 0x8005 (标准 CRC16)
- **初始值**: 0xFFFF
- **输入反转**: 否
- **输出反转**: 否
- **结果异或值**: 无

### 文件大小限制

- 最小: 无限制
- 最大: 256 MB (0x10000000 字节)

超过限制的文件将被拒绝处理。

### 平台兼容性

工具使用标准 C 库编写，支持所有主流 Linux 发行版和 POSIX 兼容系统。数据采用小端序存储，在大端序平台上使用需注意字节序转换。

## 文件清单

```
sign_tools/
├── Makefile                   # 顶层构建文件
├── Kconfig                    # 配置文件
├── output/                    # 输出目录
│   ├── ems_sign              # 打包工具
│   └── unsign_demo           # 解包工具
└── ems_sign/                 # 源代码目录
    ├── Makefile              # 子目录构建文件
    ├── ems_sign.h            # 头文件定义
    ├── ems_sign.c            # 打包工具源码
    ├── unsign_demo.c         # 解包工具源码
    └── crc16.c               # CRC16 算法实现
```

## 许可

该工具是 AM62X SDK 的组成部分，遵循 SDK 整体许可协议。
