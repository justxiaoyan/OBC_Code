#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

# Keep the child build independent from the caller's current directory. The
# top-level appfs Makefile can therefore invoke this script uniformly.
case "${1:-all}" in
  clean)
    make -C "$SCRIPT_DIR" clean
    ;;
  install)
    make -C "$SCRIPT_DIR" clean
    make -C "$SCRIPT_DIR" install
    ;;
  all|*)
    make -C "$SCRIPT_DIR" clean
    make -C "$SCRIPT_DIR" all
    ;;
esac
