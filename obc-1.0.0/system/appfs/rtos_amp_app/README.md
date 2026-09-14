# RTOS AMP 应用工程

## 1. 工程定位

本工程是面向 VS919L 平台 Cortex-A55 的裸机 AMP（Asymmetric Multi-Processing）FreeRTOS 应用。工程将 AArch64 启动代码、GIC/Timer 芯片支持库（CSL）、TI 风格 OSAL 适配层和 FreeRTOS 内核组合成一个可独立链接的 A55 固件镜像。

默认构建目标是 A55 Core 5。程序依赖启动环境预先配置的处理器、内存和中断资源，不能像普通 Linux 用户态程序一样直接运行。

## 2. 目录结构

```text
.
├── autobuild.sh                 # 按依赖顺序构建全部库和应用
├── Makefile                     # 应用编译、链接和二进制生成
├── Makefile_libcsl              # CSL 静态库构建规则
├── Makefile_libosal             # OSAL 静态库构建规则
├── Makefile_freertos            # FreeRTOS 静态库构建规则
├── main_src/                    # 应用入口、链接脚本和 libc 裁剪实现
│   ├── main.c
│   ├── libc_stubs.c
│   └── linker/linker_script.ld
├── csl/                         # A55、GICv3/GIC-600、Timer 等底层 CSL
├── osal/                        # FreeRTOS/非 OSAL 适配接口及实现
└── kernel/freertos/             # FreeRTOS 内核、A55 port、UART 驱动和配置
```

`build/` 为构建生成目录，不属于源码层级。当前仓库中没有实际的 `toolchain/` 目录；交叉编译器需要由外部环境提供。

## 3. 启动和运行流程

链接脚本的入口是 `Entry`，因此启动并不是从 C 语言 `main()` 直接开始：

```text
Entry
  -> bootcode.asm
  -> 配置 EL3/EL2/EL1、GIC 系统寄存器和栈
  -> start_boot()
  -> 清零 .bss、搬移 .data
  -> CSL_mmuStartup()
  -> Intc_Init(coreId)
  -> main()
  -> vTaskStartScheduler()
```

`csl/arch/a55/src/startup/aarch64/bootcode.asm` 负责异常级切换、GIC Redistributor 唤醒、栈初始化和中断屏蔽。`csl_a55v8_startup.c` 从 `MPIDR_EL1` 获取当前 Core ID，并把它传入后续初始化。GIC 的部分初始化在 AMP 场景下依赖上游启动环境，不能假设每个核都可以独立完成全套 GIC 初始化。

## 4. 示例应用行为

`main_src/main.c` 的示例应用执行以下操作：

1. 初始化 VS919L PL011 UART，并输出 `ayan rtos app running`。
2. 创建两个 FreeRTOS 任务，优先级均为 3，任务栈大小均为 `2 * 1024`。
3. `task1` 每 1000 tick 向 `0x89000008` 写入递增计数，并输出 `rtos task1 running`。
4. `task2` 每 2000 tick 向 `0x8900000C` 写入递增计数，并输出 `rtos task2 running`。
5. 调用 `vTaskStartScheduler()` 进入调度。

这两个地址属于应用与其他核或外设之间约定的共享数据区域，使用前应确认 AMP 内存规划和访问权限。

## 5. 平台地址和链接布局

### 5.1 固定硬件地址

工程针对 VS919L 的固定地址实现了 UART 和 GIC：

| 资源 | 地址或参数 |
| --- | --- |
| GIC Distributor | `0x0D000000` |
| GIC Redistributor | `0x0D040000` |
| GIC SGI/PPI | `0x0D050000` |
| GICX offset | `0x00020000` |
| PL011 UART | `0x0C09A000` |
| IOMUX | `0x0210E000` |
| UART9 TX/RX offset | `0x414` / `0x418` |
| UART 时钟/波特率 | `100 MHz` / `115200` |

这些地址来自 `boot_defs.h` 和 `kernel/freertos/FreeRTOS-Kernel/vs_uart/vs_uart.c`，不是通用的 PL011 驱动配置。

### 5.2 镜像内存布局

`main_src/linker/linker_script.ld` 使用：

```text
DDR_MPU1 ORIGIN = 0x80000000
DDR_MPU1 LENGTH = 0x08000000
```

因此默认链接范围是 `0x80000000` 到 `0x87FFFFFF`。脚本依次放置 `.text.el3`、`.text.csl_a55_startup`、`.text`、`.rodata`、`.data_buffer`、`.data`、异常展开表、`.bss`、`.heap` 和 `.stack`。链接脚本定义的 `__STACK_SIZE` 为 `0x20000`，启动汇编中的 `CSL_STACK_SIZE` 为 `0x10000`；修改栈规划时需要同时检查两处定义。

## 6. FreeRTOS 配置

配置文件为 `kernel/freertos/config/FreeRTOSConfig.h`。通过以下宏选择目标核：

```text
__BUILD_A55_CORE0__ ... __BUILD_A55_CORE5__
```

当前 `Makefile` 设置 `CORE_ID := 5`，并定义 `__BUILD_A55_CORE5__`。主要配置如下：

| 配置项 | 当前值 |
| --- | --- |
| Tick rate | `1000 Hz` |
| Tick clock | `3 MHz` |
| 最大任务优先级 | `16` |
| 最小栈 | `1024` |
| FreeRTOS heap | `3 MiB` |
| 调度方式 | 抢占式，关闭 time slicing |
| 最大 syscall 中断优先级 | `0xE0` |
| 启用对象 | mutex、recursive mutex、counting semaphore、task notification |
| 内存方式 | 动态分配和静态分配均启用 |

各核使用不同的 Timer ID：Core0 至 Core5 分别为 6、7、8、9、10、11。

## 7. CSL 和 OSAL 分层

### CSL

`csl/` 提供与芯片相关的底层能力，包括：

- A55 Cache、系统寄存器和 CPU ID 操作；
- AArch64 启动和异常级切换；
- GICv3/GIC-600 的中断类型、优先级、亲和性、pending、enable/disable 操作；
- Timer 的 enable/disable、模式、计数宽度、预分频、load/reload、状态和中断控制。

关键实现位于 `csl/arch/a55/src/csl_a55.c`、`csl/arch/a55/src/interrupt.c`、`csl/src/ip/arm_gic/V3/Priv/csl_gic.c` 和 `csl/src/ip/timer/V0/Priv/timer.c`。

### OSAL

`osal/` 为上层驱动提供统一接口，主要头文件包括 `TaskP.h`、`HwiP.h`、`TimerP.h`、`SemaphoreP.h`、`MutexP.h`、`DebugP.h`、`LoadP.h` 和 `RegisterIntr.h`。FreeRTOS 实现在 `osal/src/freertos/`，非 OS 实现在 `osal/src/nonos/`，A55 架构辅助代码位于 `osal/arch/core/a55/`，VS919L 默认 Timer 配置位于 `osal/soc/vs919l/`。

VS919L 默认资源上限为：Timer 16、FreeRTOS semaphore 256、Hwi 64、software timer 16、task 128、mutex 32。

当前示例应用直接调用 FreeRTOS API，没有使用 OSAL 的 `TaskP`/`TimerP`/`HwiP` 封装；OSAL 主要作为库和后续驱动适配层提供。

## 8. AMP 适配关注项

将工程移植到新的 SoC、A55 集群或 AMP 启动器时，不能只替换串口地址。建议按照下面的顺序逐项确认，并明确每项资源由哪个核、哪个固件拥有。

### 8.1 启动汇编和异常级

- 确认复位向量、链接入口 `Entry`、启动镜像加载地址与启动器跳转地址一致。
- 检查 `bootcode.asm` 中的 EL3/EL2/EL1 切换、`SCTLR_ELx`、`CPACR_EL1`、`DAIF`、栈指针和异常向量 `VBAR_ELx` 配置。
- 为每个 AMP 核分配独立的启动栈，不能让多个核共用同一栈；确认栈顶对齐满足 AArch64 ABI。
- 明确启动器传入的寄存器参数、核 ID 获取方式和启动后 C 入口约定。本工程通过 `MPIDR_EL1` 获取 Core ID，并据此选择 Timer 配置。
- 核启动、停机、复位释放和异常处理应有明确路径；核没有被启动器释放时，不能仅靠应用代码解决。
- 检查同步异常、IRQ、FIQ、SError 的入口是否有效，避免启动早期中断到达时跳转到未初始化的向量表。

### 8.2 链接脚本和内存规划

- 重新规划代码、只读数据、已初始化数据、`.bss`、heap、stack、启动表和异常表的放置区域，确认各段不重叠。
- 区分“镜像加载地址（LMA）”和“运行地址（VMA）”，确认启动器、`objcopy` 生成的 BIN 以及链接脚本使用同一套地址。
- 为不同 AMP 核、Linux、共享内存、设备内存和 DMA 缓冲区划分互不覆盖的物理地址范围；在内存表中记录所有者和大小。
- 检查 DDR 可用范围、地址宽度、对齐限制以及安全/非安全世界访问权限。本工程默认使用 `0x80000000-0x87FFFFFF`，不能直接套用到其他内存布局。
- 若启动代码搬移 `.data` 或清零 `.bss`，要确认源地址、目标地址和长度符号正确，且不会清掉其他核或启动器使用的共享区域。

### 8.3 MMU、Cache 和内存属性

- 根据 SoC 地址图重新生成页表，确认 `TTBR`、`TCR`、`MAIR`、Granule、地址空间大小和页表内存本身的位置。
- DDR 代码和数据通常配置为 Normal memory；UART、GIC、Timer、复位控制器等配置为 Device memory，不能用普通缓存属性访问。
- 确认 MMU 开启前后的地址是否一致，开启 I-Cache、D-Cache、分支预测和内存屏障的顺序符合启动规范。
- AMP 共享缓冲区必须约定缓存策略。若不同核的缓存不一致，需要使用 cache clean/invalidate、`DMB`/`DSB`/`ISB` 和生产者/消费者状态协议；`volatile` 本身不能解决缓存一致性。
- DMA 缓冲区要满足设备要求的对齐、可见性和所有权切换，避免 CPU cache 中的旧数据覆盖 DMA 结果。

### 8.4 SoC Core、时钟、复位和电源域

- 核对 `MPIDR_EL1` 的 affinity 与 SoC 逻辑 Core 编号的映射，确认 `CORE_ID`、Timer ID、中断目标核和启动寄存器使用同一编号体系。
- 核对 A55 cluster/core 的复位释放、启动地址、低功耗唤醒、时钟门控和电源域寄存器；必要时确认启动地址寄存器的高低 32 位写入顺序。
- 核对 CPU、GIC、Timer、UART、IOMUX、DDR 和外设总线的时钟源、分频、门控和复位状态。时钟频率变化会直接影响 baudrate 和 FreeRTOS tick。
- 明确哪个固件负责初始化这些寄存器，避免 SPL、Bootloader、Linux 和 RTOS 重复复位同一硬件。
- 检查 watchdog、异常复位原因和电源管理策略，防止 RTOS 启动后被未配置的看门狗复位。

### 8.5 GIC 和中断路由

- 重新确认 GIC Distributor、Redistributor、CPU interface 的基地址和每个核的 Redistributor stride；本工程当前使用 `0x0D000000`、`0x0D040000` 和 `0x0D050000`。
- 为每个 SPI/PPI/SGI 明确中断号、触发类型、优先级、目标核和归属固件，避免 Linux 与 RTOS 同时管理同一个中断。
- 确认 Redistributor 唤醒、system register interface、优先级掩码、IRQ/FIQ 屏蔽、EOI/Deactivate 流程正确。
- AMP 场景下通常只允许一个管理者初始化 Distributor；RTOS 侧应避免把其他核或 Linux 已经配置好的全局 GIC 状态清掉。
- 校对 FreeRTOS 的 `configMAX_SYSCALL_INTERRUPT_PRIORITY` 与 GIC 优先级编码，确保在 ISR 中调用 RTOS API 的中断不会高于系统调用限制。
- 逐个验证 Timer、UART、Mailbox/IPC 和外设中断，确认“产生一次、响应一次、清除一次”，防止中断风暴或丢中断。

### 8.6 Timer 和 FreeRTOS Tick

- 确认系统计数器频率、Timer 外设时钟、预分频、装载值、溢出宽度和中断号；本工程配置为 `3 MHz` tick clock、`1000 Hz` tick rate。
- 每个运行实例使用独立的 Timer 或明确共享 Timer 的仲裁方式。本工程 Core0 至 Core5 使用 Timer ID 6 至 11。
- 确认 Timer 的 raw/status/clear 寄存器顺序、自动重装载行为和启动时的 pending 状态，避免刚开调度就进入连续 tick 中断。
- 若 SoC 的计数频率改变，需要同时修改 Timer 初始化、FreeRTOS tick 换算和超时测试，不能只修改一个宏。

### 8.7 UART、IOMUX 和调试通道

- 核对 UART 基地址、寄存器偏移、FIFO/状态位、时钟、复位、IOMUX pinmux、电平和波特率。本工程使用 PL011 `0x0C09A000`、IOMUX `0x0210E000`、100 MHz 时钟和 115200 波特率。
- 确认 UART 是否被 Bootloader、Linux 或其他 AMP 核占用；共享串口输出需要锁、分时或独立通道，否则日志会交错。
- 早期启动日志应使用最小轮询发送路径，等 MMU、FreeRTOS 和中断全部稳定后再启用复杂驱动。
- 对没有完整 libc 的裸机环境，检查 `printf`、`vsnprintf` 等裁剪实现支持的格式和栈占用，避免调试代码本身引入异常。

### 8.8 FreeRTOS port 和资源隔离

- 确认 A55 port 的上下文保存/恢复、异常返回、临界区、调度器启动和 tick ISR 与当前异常级匹配；不能只替换 `FreeRTOSConfig.h`。
- 为每个核单独配置 heap、任务栈、Timer、中断和任务优先级资源，避免多个镜像使用同一 heap 或同一静态控制块。
- 核对 FPU/SIMD 上下文是否需要保存，确认编译器 ABI、`-mcpu`、浮点选项与 port 实现一致。
- 明确动态内存、C 库、日志缓冲区和驱动全局变量的所有权。AMP 不是对称多核 SMP，不能默认所有 FreeRTOS 对象都能跨核直接访问。
- 设备驱动、DMA 通道、Mailbox、GPIO、Timer、UART 等资源应建立“单一拥有者”清单；跨核访问通过 IPC 协议完成。

### 8.9 核间通信和共享内存协议

- 预先划分共享内存的地址、大小、缓存属性、数据结构版本和初始化者，禁止直接复用某个核的 `.bss` 或 heap。
- 为状态、队列、doorbell、Mailbox 等对象定义生产者/消费者、超时、重试、复位恢复和版本兼容规则。
- 对共享计数器和标志使用明确宽度、对齐和原子访问要求，并在写数据、更新状态、通知对端之间插入正确的内存屏障。
- 验证一方复位后另一方是否能重新握手，避免共享区残留旧 magic、旧指针或旧缓存导致死锁。

### 8.10 构建、镜像加载和联调

- 工具链、ABI、汇编器、链接器和 `objcopy` 必须与 AArch64 目标一致；检查启动汇编是否实际参与链接，入口符号是否为 `Entry`。
- 每次切换 Core 时同步修改 `CORE_ID`、编译宏、启动寄存器、Timer、中断目标和加载器配置。
- 检查 ELF 的入口、各段 VMA/LMA、栈/heap 末端、镜像大小和 BIN 装载范围；确认镜像没有覆盖共享内存或其他固件。
- 使用 map 文件、`readelf -h/-S/-l`、`nm` 和反汇编检查启动入口、异常向量、段权限及关键符号地址。
- 建议按“串口轮询 -> 读取 MPIDR -> MMU/cache -> Timer -> GIC -> 单任务 -> 调度器 -> IPC/DMA”的顺序逐步启用功能，每一步都保留可定位的日志。

## 9. 构建方法

确保 PATH 中存在 `aarch64-none-elf-gcc` 和 `aarch64-none-elf-ar`，然后在工程根目录执行：

```bash
sh autobuild.sh
```

脚本按以下顺序构建，保证库依赖关系正确：

```bash
make -f Makefile_libcsl csl_clean install
make -f Makefile_libosal clean install
make -f Makefile_freertos freertos_clean install
make -f Makefile clean all
```

最终产物位于 `build/`：

```text
build/rtos_lib/libcsl_a55.a
build/rtos_lib/libosal_a55.a
build/rtos_lib/freertos_a55.a
build/main/rtos_app.elf
build/main/rtos_app.bin
```

应用使用 `--specs=nosys.specs`，并以 `-ffreestanding -nostdlib -fno-builtin` 方式链接。`main_src/libc_stubs.c` 仅提供有限的 libc 函数和有限格式的 `vsnprintf`，不能按完整 glibc 行为使用。

## 10. 切换目标 Core

修改根目录 `Makefile` 中的 `CORE_ID`，使其取值为 0 至 5。该变量会映射为对应的 `__BUILD_A55_CORE<n>__` 宏，进而选择 FreeRTOS 的核相关 Timer 配置。切换后应确认：

- AMP 启动器确实把镜像加载到目标核；
- 目标核的异常级、栈和 GIC Redistributor 已准备好；
- 该核使用的 Timer ID 没有与其他运行实例冲突；
- 链接地址和共享内存区域符合系统整体内存规划。

## 11. 构建限制和验证建议

- 工程依赖外部 AArch64 bare-metal 工具链，源码目录不包含工具链。
- `Makefile` 中存在 `-Iosal/include` 和旧的 `portable/GCC/ARM_CA55_64_BIT` 路径配置；当前 A55 port 的实际目录是 `kernel/freertos/portable/GCC/a55`。如果构建报头文件或 port 路径错误，应先核对这两处配置。
- UART、GIC 和共享地址均为 VS919L 固定硬件地址，移植到其他 SoC 前必须重新核对地址和初始化顺序。
- 这是裸机 AMP 镜像，验证应在对应的启动加载器、内存保护、GIC 配置和目标硬件上进行；Linux 主机只能完成静态检查或交叉编译，不能直接执行生成的 ELF。
- 建议构建后检查 `rtos_app.elf` 的入口是否为 `Entry`、链接区间是否落在 `0x80000000-0x87FFFFFF`，并确认 `.bss`、heap、stack 没有超出预留空间。

## 12. 变更记录

- 2026-03-24：创建工程说明。
- 2026-03-31：增加静态库构建 Makefile 和 RTOS 应用示例。
