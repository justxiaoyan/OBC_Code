#!/bin/sh
set -eu
PARENT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
case "${1:-all}" in
  clean)
    make -C "$PARENT_DIR" mbw_clean
    ;;
  install)
    make -C "$PARENT_DIR" mbw
    ;;
  all|*)
    make -C "$PARENT_DIR" mbw
    ;;
esac
