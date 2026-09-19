#!/bin/sh
set -eu
PARENT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
case "${1:-all}" in
  clean)
    make -C "$PARENT_DIR" busybox_clean
    ;;
  install)
    make -C "$PARENT_DIR" busybox
    ;;
  all|*)
    make -C "$PARENT_DIR" busybox
    ;;
esac
