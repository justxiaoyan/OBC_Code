# OBC SDK

OBC SDK 是一个面向嵌入式 Linux 板卡的统一构建框架。它使用 Kconfig 管理平台、工具、应用和内核模块配置，并通过 Makefile 组织以下构建阶段：

- TI AM62x 启动链（R5 Loader、A53 U-Boot、设备树）
- Linux Kernel
- 基于 BusyBox 的根文件系统
- appfs 应用和第三方组件
- 内核模块
- OBC 签名工具和工厂镜像打包工具

当前版本：`obc-1.0.0`（`CONFIG_OBC_SDK_VERSION="1.0.0"`）。

> 当前实现状态：顶层 Makefile 已接通并可实际构建的是 **AM62x**。Kconfig 中仍保留 `imx6ull`、`rv1106`、`rk3562` 和 `platdemo` 选项，但这些平台尚未接入顶层构建分发逻辑；选择后执行构建目标会报告 `unsupported_platform`。下文的完整构建命令以 AM62x 为例。

## 目录结构

```text
OBC_Code/
├── README.md
├── obc-1.0.0/
│   ├── Makefile                 # 顶层构建入口
│   ├── Kconfig                  # 平台和组件配置
│   ├── platform_config/         # 各平台 defconfig 和 SDK 子配置
│   ├── scripts/                 # 通用及平台构建规则
│   │   └── am62x/               # AM62x loader/U-Boot/Kernel/rootfs 规则
│   ├── bootloader/obcbase/      # OBC U-Boot 扩展代码
│   ├── dts/am62x/               # AM62x 设备树源文件
│   ├── rootfs/                  # BusyBox、基础 rootfs 和打包脚本
│   ├── system/appfs/            # 可选应用
│   ├── system/third-part/       # 可选第三方组件
│   ├── module/                  # 可选内核模块
│   ├── tools/sign_tools/        # obc_sign/unsign_demo
│   ├── tools/mkkimg/            # 工厂镜像打包/解包工具
│   ├── tools/cgroup-tool/       # autocgroup、bsp_mem 等工具源码
│   └── output/                  # 构建过程中生成的文件（不应提交）
└── obc_sdk/                     # 外部 SDK（默认位于 obc-1.0.0/../obc_sdk）
    └── am62x_sdk_source/
        ├── uboot/
        └── kernel/
```

## 构建前准备

### 主机依赖

```sh
sudo apt install build-essential gcc g++ make \
    kconfig-frontends device-tree-compiler fakeroot e2fsprogs
```

其中：

- `kconfig-mconf` 由 `kconfig-frontends` 提供，用于 `make menuconfig`。
- `cpp` 和 `dtc` 用于编译 AM62x 设备树。
- `fakeroot`、`mkfs.ext2` 用于生成根文件系统镜像。

构建 AM62x Loader 还要求主机能找到 `arm-none-eabi-gcc`：

```sh
command -v arm-none-eabi-gcc
command -v kconfig-mconf
command -v dtc
command -v mkfs.ext2
```

### 外部 SDK 和交叉工具链

AM62x 默认配置使用：

```text
SDK 名称：       am62x_sdk_source
交叉编译前缀：   aarch64-ca53-linux-gnu-
目标架构：       arm64
```

推荐目录布局：

```text
/home/shanyan-hu/justxiaoyan/OBC_Code/
├── obc-1.0.0/
└── obc_sdk/
    └── am62x_sdk_source/
        ├── uboot/
        ├── kernel/
        └── ti-k3-boot-firmware-*/   # 或 uboot/binman-fake/
```

`OBC_SDK_DIR` 的查找顺序为：

1. `../obc_sdk/am62x_sdk_source`
2. `../am62x_sdk_source`
3. `../../am62x_sdk_source`

SDK 必须至少包含 `uboot/` 和 `kernel/` 目录；构建 Loader 时还需要 TI K3 firmware 目录。交叉编译器必须位于 `PATH` 中，或通过环境配置使 `aarch64-ca53-linux-gnu-gcc` 可执行。

## 配置

### 选择平台

```sh
cd /home/shanyan-hu/justxiaoyan/OBC_Code/obc-1.0.0
make platform
```

菜单当前列出：

| 选项 | 平台 | 顶层构建状态 |
| ---: | --- | --- |
| 1 | `imx6ull` | 仅保留配置，未接入当前顶层构建 |
| 2 | `rv1106` | 仅保留配置，未接入当前顶层构建 |
| 3 | `rk3562` | 仅保留配置，未接入当前顶层构建 |
| 4 | `am62x` | 当前已接入 |
| 5 | `platdemo` | 缺少对应 defconfig，不能使用 |

`make platform` 会：

1. 将 `platform_config/<platform>/<platform>_defconfig` 复制为工程根目录 `.config`。
2. 检查外部 SDK 的根目录、`uboot/` 和 `kernel/`。
3. 将平台 SDK 配置复制到外部 SDK 的 `uboot/.config` 和 `kernel/.config`（文件存在时）。

它不会编译源码，也不会生成 `output/` 镜像。

### 使用 menuconfig

```sh
make menuconfig
```

如果 `kconfig-mconf` 不在 `PATH` 中，可以显式指定：

```sh
make KCONFIG_MCONF=/path/to/kconfig-mconf menuconfig
```

可配置项目包括：

- 平台、工具链和镜像文件名
- appfs 应用
- iperf、mbw、GDB、audit、BusyBox、v4l-utils
- `shared_memory` 内核模块

## AM62x 构建流程

首次构建建议执行：

```sh
cd /home/shanyan-hu/justxiaoyan/OBC_Code/obc-1.0.0

# 1. 选择 AM62x 并初始化 .config
make platform                 # 选择 4

# 2. 按需调整工具、appfs 和模块
make menuconfig

# 3. 构建签名和工厂镜像工具
make tools

# 4. 构建全部组件
make all
```

`make all` 的实际依赖顺序为：

```text
tools → loader → uboot → kernel → rootfs → system → module → factory
```

Loader 和 U-Boot 会临时修改外部 U-Boot SDK 并在退出时恢复，因此顶层 `all` 被声明为不可并行执行。

### 单独构建目标

```sh
make tools       # sign_tools 和 mkkimg
make loader      # AM62x Cortex-R5 Loader，并签名
make uboot       # A53 U-Boot、tispl 和设备树，并签名
make kernel      # Image.gz，并签名
make rootfs      # BusyBox/rootfs.ext2，并签名
make system      # appfs 和第三方组件
make module      # 编译已启用的内核模块
make factory     # 将已签名分区镜像打包为工厂镜像
```

查看所有顶层目标：

```sh
make help
```

## 输出文件

构建过程中会创建以下目录：

```text
output/
├── tmp/                         # 标准化的中间镜像
│   ├── tiboot3.bin
│   ├── tispl.bin
│   ├── u-boot.img
│   ├── k3-am625-sk.dtb
│   ├── Image.gz
│   └── rootfs.squashfs
├── image/                       # 带 OBC 头的分区镜像
│   ├── am62x-loader.bin
│   ├── am62x-teeos.bin
│   ├── am62x-fdt.bin
│   ├── am62x-uboot.bin
│   ├── am62x-kernel.bin
│   ├── am62x-rootfs.bin
│   └── am62x-factory.bin
├── appfs/                       # appfs 安装目录（单独执行 appfs install 时使用）
└── modules/                     # 模块安装目录
```

启动链的标准文件名为：

```text
output/tmp/tiboot3.bin
output/tmp/tispl.bin
output/tmp/u-boot.img
```

AM62x GP 配置默认使用 `tispl.bin_unsigned` 和 `u-boot.img_unsigned` 作为 U-Boot 源文件，安装阶段统一改名为上面的标准文件名。Loader 使用 R5 binman 生成的 `tiboot3-am62x-gp-evm.bin`，安装时改名为 `tiboot3.bin`。

注意：当前 `CONFIG_ROOTFS_BIN_NAME` 默认是 `rootfs.squashfs`，但 `rootfs/pack_rootfs.sh` 实际通过 `mkfs.ext2` 生成 `rootfs.ext2`，随后由 AM62x 规则复制并以 `rootfs.squashfs` 文件名放入 `output/tmp`。文件名和文件系统格式目前并不一致。

## 签名和工厂镜像

### OBC 签名格式

`tools/sign_tools` 生成的文件由以下部分组成：

```text
[512 字节 OBCFS 头] + [原始二进制数据]
```

头部包含魔数 `OBCFS`、原始文件大小、CRC16、文件名和 `head_write_flag`。各构建阶段的标志如下：

| 镜像 | `head_write_flag` |
| --- | ---: |
| Loader、Rootfs | 0 |
| FDT、TEE-OS、U-Boot、Kernel | 1 |

手工使用：

```sh
cd tools/sign_tools
make
./output/obc_sign input.bin output.bin       # 不写入头部标志
./output/obc_sign -h input.bin output.bin    # head_write_flag=1
./output/unsign_demo output.bin              # 查看头部
./output/unsign_demo output.bin raw.bin      # 解包并校验 CRC16
```

### 工厂镜像

`make factory` 使用 `tools/mkkimg/output/mkkimg` 扫描 `output/image/`，只识别当前平台前缀下的以下文件：

```text
<platform>-loader.bin
<platform>-teeos.bin
<platform>-fdt.bin
<platform>-uboot.bin
<platform>-kernel.bin
<platform>-rootfs.bin
```

最多打包 6 个文件，输出：

```text
output/image/am62x-factory.bin
```

验证或解包工厂镜像：

```sh
tools/mkkimg/output/umkkimg output/image/am62x-factory.bin --info
tools/mkkimg/output/umkkimg output/image/am62x-factory.bin --verify
tools/mkkimg/output/umkkimg output/image/am62x-factory.bin --extract \
    --output /tmp/am62x-extracted
```

## appfs、第三方组件和模块

### appfs 应用

`system/appfs/Kconfig` 当前提供以下可选项：

```text
tools-mem、tools-smart、tools-priority、LiteMonitor、StockMonitor、
lvgl、camera-fb、pxp_demo、printf_test、upgrade
```

启用后，`make system` 会调用 `system/appfs/Makefile` 构建相应子目录。若要显式安装到 `output/appfs`，可执行：

```sh
make -C system/appfs install OBC_TOP_DIR=$PWD
```

部分历史应用仍使用固定的本机编译器或绝对路径交叉工具链，启用前请检查对应子目录的 Makefile。

### 第三方组件

可选组件和源码归档位于 `system/third-part/`：

| Kconfig 选项 | 版本/目录 |
| --- | --- |
| `CONFIG_TOOLS_IPERF` | iperf 3.18 |
| `CONFIG_TOOLS_MBW` | mbw（压缩包 `mbw-master.zip`） |
| `CONFIG_TOOLS_GDB` | GDB 10.2 |
| `CONFIG_TOOLS_AUDIT` | audit 3.1.4 |
| `CONFIG_TOOLS_BUSYBOX` | BusyBox 1.36.1 |
| `CONFIG_TOOLS_V4L_UTILS` | v4l-utils 1.22.1 |

这些组件使用 `.config` 中的 `CONFIG_OBC_SDK_COMP` 推导交叉工具链前缀。启用组件后执行：

```sh
make system
```

### shared_memory 内核模块

在 menuconfig 中启用：

```text
Kernel Modules → Enable shared_memory driver (/dev/shared_memory)
```

然后编译：

```sh
make module
```

模块源码位于 `module/shared_memory/`，默认目标名为 `multicore-shared-memory.ko`，使用外部 SDK 的 `kernel/` 目录进行内核模块编译。驱动要求设备树提供 `memory-region`，并提供：

- `/dev/shared_memory` 字符设备
- `mmap()` 映射保留内存
- `SHMEM_IOC_GET_REGION_COUNT`
- `SHMEM_IOC_GET_REGION_INFO`

详细的设备树和用户态接口示例见 [`module/shared_memory/README.md`](obc-1.0.0/module/shared_memory/README.md)。当前顶层 `module` 目标负责编译，不会自动调用子模块的 `install` 目标；需要安装到 `output/modules` 时可执行：

```sh
make -C module/shared_memory install \
    KERNEL_SDK_DIR=$PWD/../obc_sdk/am62x_sdk_source/kernel \
    MODULE_OUTPUT_DIR=$PWD/output/modules
```

## 清理

```sh
make clean
```

也可以按阶段清理：

```sh
make loader_clean
make uboot_clean
make kernel_clean
make rootfs_clean
make system_clean
make module_clean
make tools_clean
make factory_clean
```

这些目标会清理相应的构建目录和 OBC 输出；`uboot_clean`/`loader_clean` 还会清理外部 U-Boot SDK 的构建结果。

## 已知限制和注意事项

1. 顶层 Makefile 目前只有 `am62x` 分支；其他平台配置不能按当前 `make all` 流程构建。
2. `platdemo` 在 `platform_config/` 下没有对应的 `platdemo_defconfig`。
3. `make platform` 会覆盖工程 `.config`，并可能覆盖外部 SDK 的 `uboot/.config`、`kernel/.config`，请先备份手工修改。
4. 当前没有顶层 `make install` 或 `make pack` 目标；各阶段构建目标会直接把签名结果写入 `output/tmp` 和 `output/image`，工厂镜像目标名为 `make factory`。
5. 根文件系统默认文件名为 `rootfs.squashfs`，实际内容是 ext2 镜像，见上文说明。
6. 外部 SDK、交叉工具链、TI K3 firmware 和 `arm-none-eabi-gcc` 均需用户预先准备，OBC 不会自动 clone 或下载这些资源。

## 相关文档

- [`obc-1.0.0/doc/obc-sdk-guide.md`](obc-1.0.0/doc/obc-sdk-guide.md)：平台配置流程和 SDK 路径说明
- [`obc-1.0.0/doc/buildroot-am62x-sdk-guide.md`](obc-1.0.0/doc/buildroot-am62x-sdk-guide.md)：Buildroot AM62x 方案
- [`obc-1.0.0/tools/sign_tools/README.md`](obc-1.0.0/tools/sign_tools/README.md)：OBCFS 签名格式和工具用法
- [`obc-1.0.0/tools/cgroup-tool/README.md`](obc-1.0.0/tools/cgroup-tool/README.md)：autocgroup、bsp_mem 用法
- [`obc-1.0.0/module/shared_memory/INTEGRATION.md`](obc-1.0.0/module/shared_memory/INTEGRATION.md)：shared_memory 模块集成背景（其中部分历史命令以当前顶层 Makefile 为准）
