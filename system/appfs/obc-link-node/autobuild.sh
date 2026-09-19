#!/bin/sh
set -eu
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
CC_CMD=${CC:-${CROSS_COMPILE:-}gcc}
DEST=${DESTDIR:-${APPFS_OUTPUT_DIR:-}}
case "${1:-all}" in
  clean)
    make -C "$SCRIPT_DIR" clean
    ;;
  install)
    make -C "$SCRIPT_DIR" all CC="$CC_CMD"
    test -n "$DEST"
    mkdir -p "$DEST"
    install -m 755 "$SCRIPT_DIR/bin/obc-link-node" "$DEST/"
    ;;
  all|*)
    make -C "$SCRIPT_DIR" all CC="$CC_CMD"
    ;;
esac
