#!/bin/sh
set -eu
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
case "${1:-all}" in
  clean)
    make -C "$SCRIPT_DIR" clean
    ;;
  install)
    make -C "$SCRIPT_DIR" install APPFS_OUTPUT_DIR="${DESTDIR:-${APPFS_OUTPUT_DIR:-$SCRIPT_DIR/../../output/appfs}}"
    ;;
  all|*)
    make -C "$SCRIPT_DIR" all
    ;;
esac
