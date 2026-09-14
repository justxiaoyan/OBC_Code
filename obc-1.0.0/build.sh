#!/bin/sh

set -eu

OBC_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
OBC_CONFIG="${OBC_DIR}/.config"

if [ ! -f "${OBC_CONFIG}" ]; then
	printf 'ERROR: OBC configuration not found: %s\n' "${OBC_CONFIG}" >&2
	printf 'Run "make platform" before using this build script.\n' >&2
	exit 1
fi

PLATFORM_NAME=$(sed -n 's/^CONFIG_PLATFORM_CONFIG="\([^"]*\)"$/\1/p' "${OBC_CONFIG}")
if [ -z "${PLATFORM_NAME}" ]; then
	printf 'ERROR: CONFIG_PLATFORM_CONFIG is missing from %s\n' "${OBC_CONFIG}" >&2
	printf 'Run "make platform" to select a platform.\n' >&2
	exit 1
fi

case "${PLATFORM_NAME}" in
	am62x)
		AM62X_TOOLCHAIN_ROOT=${AM62X_TOOLCHAIN_ROOT:-"${HOME}/toolchain/am62x-host"}
		AM62X_TOOLCHAIN_BIN="${AM62X_TOOLCHAIN_ROOT}/bin"

		if [ ! -d "${AM62X_TOOLCHAIN_BIN}" ]; then
			printf 'ERROR: AM62x toolchain directory not found: %s\n' "${AM62X_TOOLCHAIN_BIN}" >&2
			exit 1
		fi
		if [ ! -x "${AM62X_TOOLCHAIN_BIN}/aarch64-ca53-linux-gnu-gcc" ]; then
			printf 'ERROR: AM62x cross compiler not found: %s\n' \
				"${AM62X_TOOLCHAIN_BIN}/aarch64-ca53-linux-gnu-gcc" >&2
			exit 1
		fi

		printf 'Building platform %s with toolchain %s\n' \
			"${PLATFORM_NAME}" "${AM62X_TOOLCHAIN_ROOT}"
		exec env PATH="${AM62X_TOOLCHAIN_BIN}:${PATH}" make -C "${OBC_DIR}" "$@"
		;;
	*)
		printf 'Building platform %s with the current host environment\n' "${PLATFORM_NAME}"
		exec make -C "${OBC_DIR}" "$@"
		;;
esac
