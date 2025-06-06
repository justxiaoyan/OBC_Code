[toc]

---

# 1、Tools

- 常用的第三方库、工具编译集合

**需要用到的依赖资源：**

```c
sudo apt install golang slapd ldap-utils kconfig-frontends
```

## 1.1、使用方法

- 根据不同的开发环境，选择不同的交叉编译工具链

```c
### GLIBC Cross Compile AArch64
EXTERNEL_DIR := $(shell pwd)
HOST         := arm-linux-gnueabihf				//可修改
CC           := $(HOST)-gcc
CXX          := $(HOST)-g++

```

- 选择需要的工具config，然后执行make

```c
cd /1-Tools
make menuconfig
make
```

## 1.2、新增支持的工具

- 增加支持的第三方工具，需要在顶层Kconfig中添加COFNIG，并在顶层Makefile中添加编译方法，根据不同的SOC

```c
//cat Kconfig                                                     

menu  "Network testing"

config TOOLS_IPERF
        bool "Build iperf"
        default n

config TOOLS_MBW
        bool "Build mbw"
        default n

config TOOLS_GDB
        bool "Build gdb"
        default n

config TOOLS_AUDIT
        bool "Build audit"
        default n

endmenu

```



# 2、Imx6ull-Board

- 野火imx6ull编译源码库