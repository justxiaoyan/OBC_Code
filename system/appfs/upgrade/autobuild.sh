#!/bin/sh
set -eu
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
DEST=${DESTDIR:-${APPFS_OUTPUT_DIR:-}}
case "${1:-all}" in
  clean)
    make -C "$SCRIPT_DIR" clean CROSS_COMPILE="${CROSS_COMPILE:-}"
    ;;
  install)
    make -C "$SCRIPT_DIR" all CROSS_COMPILE="${CROSS_COMPILE:-}"
    test -n "$DEST"
    mkdir -p "$DEST"
    install -m 755 "$SCRIPT_DIR/upgrade" "$DEST/"
    ;;
  all|*)
    make -C "$SCRIPT_DIR" all CROSS_COMPILE="${CROSS_COMPILE:-}"
    ;;
esac
