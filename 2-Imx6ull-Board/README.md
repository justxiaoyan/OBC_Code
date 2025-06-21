# OBC 框架使用说明

- One-Stop Build and Compile

## 1、目录结构

```shell
.
├── Makefile                    # 顶层make，可以完整的管理整个OBC编译项目
├── module
│   ├── driver                  # 驱动编译框架
│   │   ├── adc
│   │   └── gpio
│   └── hal                     # hal层库目录
├── output
│   ├── image                   # 成果物pack后的产物
│   ├── tmp                     # 成果物临时目录
│   └── tools                   # pack工具
├── platform_config             # 平台化构建配置
│   ├── imx6ull                 # imx6ull平台
│   │   ├── imx6ull_defconfig
│   │   └── sdk_config          # sdk配置保存管理，不使用在sdk目录下的maek
│   │       ├── kernel_imx6ull_defconfig
│   │       └── uboot_imx6ull_defconfig
│   └── platform.mk
└── README.md
```

## 2、编译步骤

### 2.1、选择编译的平台

- 该步骤会将`sdk_config`下的内容拷贝到SDK目录
- 现在还有点bug，需要执行两次这个命令

```shell
make platform
```

### 2.2、执行编译

- 编译所有产物

```shell
make all
```

- 等效于

```shell
make uboot kernel rootfs
```

### 2.3、打包命令

- 对产物添加包头，并拷贝到到`tftp`目录

```shell
make pack_isntall
```

- tftp目录配置在顶层`Makefile`中

```makefile
TFTP_DEBUG_DIR := 
```

### 2.4、SDK配置

- 修改`kernel、uboot`的`config`配置，这个是临时修改，不会保存到`platform_config/sdk_config`中

```shell
make uboot_menuconfig

make kernel_menuconfig
```

- 保存`sdk config`修改，会将SDK目录下更新的`config`保存到`platform_config/sdk_config`

```shell
make saveconfig_uboot

make saveconfig_kernel
```