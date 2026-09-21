#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

case "${1:-all}" in
	clean)
		exec make -C "${SCRIPT_DIR}" clean
		;;
	install)
		exec make -C "${SCRIPT_DIR}" install \
			CROSS_COMPILE="${CROSS_COMPILE:-}" \
			DESTDIR="${DESTDIR:-}"
		;;
	all)
		exec make -C "${SCRIPT_DIR}" all \
			CROSS_COMPILE="${CROSS_COMPILE:-}"
		;;
	*)
		echo "Usage: $0 {all|install|clean}" >&2
		exit 2
		;;
esac
