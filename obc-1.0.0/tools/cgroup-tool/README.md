# autocgroup

- 该工具实现对`cgroup`组的`config`管理，支持以下参数配置：

```shell
-n cgroup_name     : cgroup name
-u cpu_percent     : CPU usage limit percentage (0-100)
-p PID             : Add pid to cgroup
-m max_memory high_memory : Memory limits in bytes
-c cpuset          : CPU set (e.g., 0-3 or 1)
-r read_bps        : Read bandwidth limit in bytes/sec
-w write_bps       : Write bandwidth limit in bytes/sec
-s show            : Show all config
```

# bsp_mem

- 该工具实现指定内存空间的申请，并循环向申请空间写入随机数，保证内存被实际访问
- 传入参数`mem_size`要求`4K`对齐，未对齐代码自动对齐

```shell
bsp_mem mem_size
```















