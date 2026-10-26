#!/bin/sh
set -eu
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
CC_CMD=${CC:-${CROSS_COMPILE:-}gcc}
DEST=${DESTDIR:-${APPFS_OUTPUT_DIR:-}}
build() {
  make -C "$SCRIPT_DIR" -j"$(nproc)" all CC="$CC_CMD"
}
case "${1:-all}" in
  clean)
    make -C "$SCRIPT_DIR" clean
    ;;
  install)
    build
    test -n "$DEST"
    mkdir -p "$DEST"
    install -m 755 "$SCRIPT_DIR/build/obc_lvgl" "$DEST/"
    ;;
  all|*)
    build
    ;;
esac
