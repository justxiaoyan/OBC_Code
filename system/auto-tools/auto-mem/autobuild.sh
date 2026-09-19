#!/bin/sh
set -eu
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
case "${1:-all}" in
  clean)
    make -C "$SCRIPT_DIR" clean
    ;;
  install)
    make -C "$SCRIPT_DIR" install DESTDIR="${DESTDIR:?DESTDIR is required}"
    ;;
  all|*)
    make -C "$SCRIPT_DIR" all
    ;;
esac
