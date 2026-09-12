# auto-priority

`auto-priority` displays Linux thread scheduling priority, policy and CPU
usage. It is selected from the OBC top-level `system` menuconfig through
`CONFIG_AUTO_TOOLS_PRIORITY`.

## Build directly

```sh
make CROSS_COMPILE=aarch64-buildroot-linux-gnu-
make install DESTDIR=/tmp/auto-tools
```

The main executable is installed as `auto-priority`. The optional demo is
available with `make demo` and is named `auto-priority_demo`.

## OBC build

Enable `Application File System -> Auto Tools -> Enable auto-priority` in
`make menuconfig`, then run:

```sh
make system
```

The staged executable is written to `output/auto-tools/auto-priority`.
