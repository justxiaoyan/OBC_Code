#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
OBC_TOP_DIR=$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)
BASE_ROOTFS="${SCRIPT_DIR}/base_rootfs"
STAGING_ROOT=${ROOTFS_STAGING_DIR:-${OBC_TOP_DIR}/output/tmp/install}
OUTPUT_FILE=${ROOTFS_OUTPUT_FILE:-${OBC_TOP_DIR}/output/tmp/rootfs.sparse.bin}
MAKE_EXT4FS=${MAKE_EXT4FS:-make_ext4fs}
ARCH=${ARCH:-arm64}
CROSS_COMPILE=${CROSS_COMPILE:-aarch64-ca53-linux-gnu-}
SYSROOT=${SYSROOT:-}
STRIP=${STRIP:-${CROSS_COMPILE}strip}
KERNEL_SDK_DIR=${KERNEL_SDK_DIR:-${OBC_TOP_DIR}/../am62x_sdk_source/kernel}
KERNEL_CONFIG=${KERNEL_CONFIG:-${OBC_TOP_DIR}/platform_config/am62x/sdk_config/kernel-am62x-defconfig}
OBCBASE_BUILD_DIR=${OBCBASE_BUILD_DIR:-${OBC_TOP_DIR}/output/tmp/obcbase-build}
OBCTOOL_BUILD_DIR=${OBCTOOL_BUILD_DIR:-${OBC_TOP_DIR}/output/tmp/obctool-build}
CONFIG_OBC_BASE=${CONFIG_OBC_BASE:-y}

if [ ! -d "${BASE_ROOTFS}" ]; then
	echo "ERROR: base rootfs directory not found: ${BASE_ROOTFS}" >&2
	exit 1
fi

if ! command -v "${MAKE_EXT4FS}" >/dev/null 2>&1; then
	echo "ERROR: make_ext4fs not found: ${MAKE_EXT4FS}" >&2
	echo "       install make_ext4fs or set MAKE_EXT4FS=/path/to/make_ext4fs" >&2
	exit 127
fi

for required in \
	"${BASE_ROOTFS}/init" \
	"${BASE_ROOTFS}/etc/init.d/rcS" \
	"${BASE_ROOTFS}/etc/init.d/rcK"; do
	if [ ! -f "${required}" ]; then
		echo "ERROR: required rootfs file not found: ${required}" >&2
		exit 1
	fi
done

if command -v file >/dev/null 2>&1; then
	base_elf=$(find "${BASE_ROOTFS}" -type f -exec file -b {} \; \
		| grep -m1 '^ELF ' || true)
	if [ -n "${base_elf}" ]; then
		echo "ERROR: base_rootfs contains generated ELF resources" >&2
		exit 1
	fi
fi

find "${BASE_ROOTFS}" -type l -print | while IFS= read -r link; do
	target=$(readlink "${link}")
	case "${target}" in
		*busybox*)
			echo "ERROR: base_rootfs contains a BusyBox artifact link: ${link}" >&2
			exit 1
			;;
	esac
done

rm -rf "${STAGING_ROOT}"
mkdir -p "${STAGING_ROOT}"
cp -a "${BASE_ROOTFS}/." "${STAGING_ROOT}/"

# Generated runtime directories never belong to the base_rootfs template.
mkdir -p \
	"${STAGING_ROOT}/bin" \
	"${STAGING_ROOT}/sbin" \
	"${STAGING_ROOT}/lib" \
	"${STAGING_ROOT}/usr/bin" \
	"${STAGING_ROOT}/usr/sbin" \
	"${STAGING_ROOT}/usr/obc/ko" \
	"${STAGING_ROOT}/dev/pts" \
	"${STAGING_ROOT}/dev/shm" \
	"${STAGING_ROOT}/proc" \
	"${STAGING_ROOT}/sys" \
	"${STAGING_ROOT}/run" \
	"${STAGING_ROOT}/tmp" \
	"${STAGING_ROOT}/mnt" \
	"${STAGING_ROOT}/root" \
	"${STAGING_ROOT}/appfs" \
	"${STAGING_ROOT}/var/log" \
	"${STAGING_ROOT}/var/tmp"
ln -s lib "${STAGING_ROOT}/lib64"

# Install the ARM64 BusyBox only after the base framework has been copied.
make -C "${SCRIPT_DIR}" busybox_install \
	ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" SYSROOT="${SYSROOT}" \
	ROOTFS_STAGING_DIR="${STAGING_ROOT}"

make -C "${SCRIPT_DIR}" obcbase_install \
	ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" SYSROOT="${SYSROOT}" \
	ROOTFS_STAGING_DIR="${STAGING_ROOT}" KERNEL_SDK_DIR="${KERNEL_SDK_DIR}" \
	KERNEL_CONFIG="${KERNEL_CONFIG}" OBCBASE_BUILD_DIR="${OBCBASE_BUILD_DIR}" \
	CONFIG_OBC_BASE="${CONFIG_OBC_BASE}"

make -C "${SCRIPT_DIR}" obctool_install \
	CROSS_COMPILE="${CROSS_COMPILE}" SYSROOT="${SYSROOT}" \
	ROOTFS_STAGING_DIR="${STAGING_ROOT}" \
	OBCTOOL_BUILD_DIR="${OBCTOOL_BUILD_DIR}"

if [ ! -x "${STAGING_ROOT}/bin/busybox" ]; then
	echo "ERROR: ARM64 BusyBox was not installed" >&2
	exit 1
fi

find_sysroot_file() {
	name=$1
	case "${name}" in
		*/*)
			if [ -e "${SYSROOT}/${name}" ] || [ -L "${SYSROOT}/${name}" ]; then
				printf '%s\n' "${SYSROOT}/${name}"
				return 0
			fi
			return 1
			;;
	esac
	for directory in "${SYSROOT}/lib" "${SYSROOT}/lib64" "${SYSROOT}/usr/lib"; do
		if [ -e "${directory}/${name}" ] || [ -L "${directory}/${name}" ]; then
			printf '%s\n' "${directory}/${name}"
			return 0
		fi
	done
	find "${SYSROOT}" -maxdepth 4 \( -type f -o -type l \) -name "${name}" -print -quit
}

copy_runtime_file() {
	relative_path=$1
	source_path="${SYSROOT}/${relative_path}"

	if [ ! -e "${source_path}" ] && [ ! -L "${source_path}" ]; then
		echo "ERROR: runtime file not found in sysroot: ${relative_path}" >&2
		exit 1
	fi

	destination_path="${STAGING_ROOT}/${relative_path}"
	mkdir -p "$(dirname "${destination_path}")"
	cp -a "${source_path}" "${destination_path}"

	if [ -L "${source_path}" ]; then
		resolved_path=$(readlink -f "${source_path}")
		resolved_relative=${resolved_path#"${SYSROOT}/"}
		if [ "${resolved_relative}" != "${relative_path}" ]; then
			copy_runtime_file "${resolved_relative}"
		fi
	fi
}

copy_needed_runtime() {
	name=$1
	source_path=$(find_sysroot_file "${name}")
	if [ -z "${source_path}" ]; then
		echo "ERROR: could not locate ${name} in sysroot ${SYSROOT}" >&2
		exit 1
	fi
	relative_path=${source_path#"${SYSROOT}/"}
	copy_runtime_file "${relative_path}"
}

if command -v readelf >/dev/null 2>&1; then
	if [ -z "${SYSROOT}" ]; then
		echo "ERROR: SYSROOT is required for the dynamic ARM64 BusyBox" >&2
		exit 1
	fi

	interpreter=$(readelf -l "${STAGING_ROOT}/bin/busybox" \
		| sed -n 's/.*Requesting program interpreter: \([^]]*\)].*/\1/p')
	if [ -n "${interpreter}" ]; then
		copy_needed_runtime "${interpreter#/}"
	fi

	needed_libraries=$(readelf -d "${STAGING_ROOT}/bin/busybox" \
		| sed -n 's/.*Shared library: \[\([^]]*\)\].*/\1/p')
	for library in ${needed_libraries}; do
		copy_needed_runtime "${library}"
	done

	for required_runtime in \
		"${STAGING_ROOT}${interpreter}" \
		"${STAGING_ROOT}/lib/libc.so.6"; do
		if [ ! -e "${required_runtime}" ] && [ ! -L "${required_runtime}" ]; then
			echo "ERROR: required runtime library missing from rootfs: ${required_runtime}" >&2
			exit 1
		fi
	done
fi

# Remove debug and linker-only sections from every ELF copied into the
# rootfs. BusyBox is normally already stripped, while the sysroot glibc
# files often contain debug sections that would otherwise dominate the image.
if command -v "${STRIP}" >/dev/null 2>&1 && command -v file >/dev/null 2>&1; then
	find "${STAGING_ROOT}" -type f -print | while IFS= read -r candidate; do
		if file -b "${candidate}" | grep -q 'ELF'; then
			case "${candidate}" in
				*.ko) "${STRIP}" --strip-debug "${candidate}" ;;
				*) "${STRIP}" --strip-unneeded "${candidate}" ;;
			esac
		fi
	done
else
	echo "WARNING: ${STRIP} or file not found; ELF files were not stripped" >&2
fi

rm -f "${STAGING_ROOT}/etc/mtab"
ln -s /proc/mounts "${STAGING_ROOT}/etc/mtab"
rm -f "${STAGING_ROOT}/var/run"
ln -s /run "${STAGING_ROOT}/var/run"

chmod 0755 \
	"${STAGING_ROOT}/init" \
	"${STAGING_ROOT}/etc/init.d/rcS" \
	"${STAGING_ROOT}/etc/init.d/rcK" \
	"${STAGING_ROOT}/etc/init.d/S01_basedev" \
	"${STAGING_ROOT}/etc/init.d/S07_net_config" \
	"${STAGING_ROOT}/etc/init.d/S09_obc_base" \
	"${STAGING_ROOT}/etc/init.d/S10_obc_startup" \
	"${STAGING_ROOT}/etc/mdev/obcblock"
chmod 0700 "${STAGING_ROOT}/root"
chmod 1777 "${STAGING_ROOT}/tmp" "${STAGING_ROOT}/var/tmp"

# Device nodes are populated by devtmpfs/mdev at boot. make_ext4fs does not
# accept device nodes in its input directory.
rm -f "${STAGING_ROOT}/dev/console"

if [ "$(id -u)" -eq 0 ]; then
	chown -hR 0:0 "${STAGING_ROOT}"
fi

mkdir -p "$(dirname "${OUTPUT_FILE}")"
temporary_output="${OUTPUT_FILE}.tmp"
rm -f "${temporary_output}"
"${MAKE_EXT4FS}" -s -l 32M "${temporary_output}" "${STAGING_ROOT}"
mv -f "${temporary_output}" "${OUTPUT_FILE}"

echo "Created ${OUTPUT_FILE}"
du -h "${OUTPUT_FILE}"
