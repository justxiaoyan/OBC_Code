# AM62x A53 Core 3 FreeRTOS Demo

本工程生成一个面向 TI AM62x Cortex-A53 core 3 的最小 FreeRTOS AMP 镜像。AM62x 的四个 A-class 核实际是 Cortex-A53，不是 Cortex-A52。

Demo 创建一个 FreeRTOS 任务，每 1000 ms 将物理地址 `0x8f000000` 处的 32 位计数器加 1。写入后会 clean 对应 cache line 到一致性点，因此另一个处理器可以用 U-Boot 的 `md.l` 观察变化。当前版本不初始化 UART，也不依赖 Linux。

## 内存和硬件资源

| 项目 | 配置 |
| --- | --- |
| 镜像入口/加载地址 | `0x84000000` |
| 镜像链接区间 | `0x84000000` - `0x87ffffff` |
| DDR MMU 映射 | `0x80000000` - `0xbfffffff`，Normal WB、Inner Shareable |
| 观测计数器 | `0x8f000000`，32 位 |
| 目标 CPU | AM62x A53 core 3，`MPIDR_EL1.Aff0 == 3` |
| GIC Distributor | `0x01800000` |
| core 0 Redistributor | `0x01880000` |
| Redistributor stride | `0x00020000` |
| FreeRTOS tick timer | `main_timer1`，`0x02410000` |
| Timer 时钟 | 25 MHz |
| Timer 中断 | DTS `GIC_SPI 121`，GIC INTID `153` |

链接区、计数器地址以及 U-Boot/其他固件使用的内存不能重叠。配套的 A53 U-Boot remoteproc 驱动会在 `rproc init 0` 时通过 TI-SCI 给空闲的 `main_timer1` 上电、选择 25 MHz 时钟源，并核对实际频率；U-Boot 自身继续使用 `main_timer0`。

## 构建

默认工具链前缀为：

```text
/home/shanyan-hu/toolchain/am62x-host/bin/aarch64-ca53-linux-gnu-
```

直接构建：

```bash
make clean
make -j2
```

也可以覆盖工具链：

```bash
make clean
make -j2 TOOLCHAIN_PREFIX=/path/to/bin/aarch64-none-elf-
```

或使用：

```bash
./autobuild.sh -j2
```

产物为：

```text
build/am62x/rtos_app.elf
build/am62x/rtos_app.bin
```

板端必须加载并启动 `rtos_app.bin`，不能把 `rtos_app.elf` 传给 `rproc load`。配套驱动把 `rproc load` 的地址作为 PSCI `CPU_ON` 启动向量，并在释放 core 3 前 clean 对应的 U-Boot data cache。

## 构建配套 U-Boot

配套修改位于以下位置：

```text
/home/shanyan-hu/justxiaoyan/OBC_Code/obc_sdk/am62x_sdk_source/uboot
/home/shanyan-hu/justxiaoyan/OBC_Code/obc-1.0.0/bootloader
/home/shanyan-hu/justxiaoyan/OBC_Code/obc-1.0.0/platform_config/am62x
```

从 OBC 工程根目录构建 A53 U-Boot：

```bash
cd /home/shanyan-hu/justxiaoyan/OBC_Code/obc-1.0.0
make uboot_build
```

构建链会将 ALIENTEK U-Boot overlay、PSCI remoteproc 驱动和配置同步到 SDK。新增节点固定为 `remoteproc0`，目标 MPIDR 为 `0x3`。驱动由运行在 core 0 上的 A53 U-Boot proper 调用 PSCI `CPU_ON`，不再使用 R5 启动阶段那个面向 A53 core 0 的 `ti_k3_arm64_rproc` 节点。

## U-Boot 启动

把 `build/am62x/rtos_app.bin` 放到 U-Boot 能读取的 FAT/ext4 分区。以下以 MMC 分区 `<dev:part>` 为例：

```text
rproc init 0
rproc list
load mmc <dev:part> 0x84000000 rtos_app.bin
mw.l 0x8f000000 0 1
rproc load 0 0x84000000 ${filesize}
rproc start 0
```

`rproc list` 中应出现 ID 0、名称 `am62x-a53-core3`。启动后间隔约一秒重复查看：

使用 TFTP 时对应命令为：

```text
rproc init 0
rproc list
tftp 0x84000000 rtos_app.bin
mw.l 0x8f000000 0 1
rproc load 0 0x84000000 ${filesize}
rproc start 0
```

不要把 `rtos_app.elf` 作为 `rproc load` 的输入；当前驱动把下载地址直接作为 core3 的执行入口，不解析或搬运 ELF 段。

启动后查看计数器：

```text
md.l 0x8f000000 1
```

数值应持续按 `0x00000001`、`0x00000002`、`0x00000003` 递增。也可以根据文件系统改用 `fatload`、`ext4load` 或 `tftp`，但加载地址必须保持为 `0x84000000`。

该 remoteproc 驱动没有实现停止已经运行的 A53 核。一次上电周期只执行一次 `rproc start 0`；需要重新加载或重新启动 demo 时，请先复位整板。若 PSCI 返回 `ALREADY_ON`，说明 core 3 已被其他固件启动，需先检查启动链的多核配置。

## 静态检查

可用以下命令核对镜像：

```bash
/home/shanyan-hu/toolchain/am62x-host/bin/aarch64-ca53-linux-gnu-readelf -h build/am62x/rtos_app.elf
/home/shanyan-hu/toolchain/am62x-host/bin/aarch64-ca53-linux-gnu-nm -n build/am62x/rtos_app.elf | head
/home/shanyan-hu/toolchain/am62x-host/bin/aarch64-ca53-linux-gnu-objdump -d build/am62x/rtos_app.elf | less
```

静态构建成功不能替代 AM62x 板上验证。当前地址规划假设至少有覆盖 `0x84000000` 到 `0x8f000003` 的 DDR，并且启动链没有把链接区和计数器地址留给 U-Boot、OP-TEE 或其他固件；如板级内存布局不同，必须同时修改链接地址、加载地址和计数器地址。
