#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
OBC_TOP_DIR=$(CDPATH= cd -- "${SCRIPT_DIR}/.." && pwd)
BASE_ROOTFS="${SCRIPT_DIR}/base_rootfs"
STAGING_ROOT=${ROOTFS_STAGING_DIR:-${OBC_TOP_DIR}/output/tmp/install}
OUTPUT_FILE=${ROOTFS_OUTPUT_FILE:-${OBC_TOP_DIR}/scripts/am62x/rootfs.cpio.gz}
ARCH=${ARCH:-arm64}
CROSS_COMPILE=${CROSS_COMPILE:-aarch64-ca53-linux-gnu-}
SYSROOT=${SYSROOT:-}
STRIP=${STRIP:-${CROSS_COMPILE}strip}

if [ ! -d "${BASE_ROOTFS}" ]; then
	echo "ERROR: base rootfs directory not found: ${BASE_ROOTFS}" >&2
	exit 1
fi

for required in \
	"${BASE_ROOTFS}/init" \
	"${BASE_ROOTFS}/etc/init.d/rcS" \
	"${BASE_ROOTFS}/etc/init.d/rcK"; do
	if [ ! -f "${required}" ]; then
		echo "ERROR: required initramfs file not found: ${required}" >&2
		exit 1
	fi
done

rm -rf "${STAGING_ROOT}"
mkdir -p "${STAGING_ROOT}"
cp -a "${BASE_ROOTFS}/." "${STAGING_ROOT}/"

# Keep the base_rootfs layout and configuration, but never reuse its ARMHF
# executables or libraries in the ARM64 image.
for directory in bin sbin usr/bin usr/sbin lib; do
	mkdir -p "${STAGING_ROOT}/${directory}"
	find "${STAGING_ROOT}/${directory}" -mindepth 1 -maxdepth 1 -exec rm -rf -- {} +
done
rm -rf "${STAGING_ROOT}/lib64"
mkdir -p "${STAGING_ROOT}/lib"
ln -s lib "${STAGING_ROOT}/lib64"

mkdir -p \
	"${STAGING_ROOT}/dev/pts" \
	"${STAGING_ROOT}/dev/shm" \
	"${STAGING_ROOT}/proc" \
	"${STAGING_ROOT}/sys" \
	"${STAGING_ROOT}/run" \
	"${STAGING_ROOT}/tmp" \
	"${STAGING_ROOT}/root" \
	"${STAGING_ROOT}/var/log" \
	"${STAGING_ROOT}/var/tmp"

# Install the ARM64 BusyBox only after the base framework has been copied.
make -C "${SCRIPT_DIR}" busybox_install \
	ARCH="${ARCH}" CROSS_COMPILE="${CROSS_COMPILE}" SYSROOT="${SYSROOT}" \
	ROOTFS_STAGING_DIR="${STAGING_ROOT}"

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

	readelf -d "${STAGING_ROOT}/bin/busybox" \
		| sed -n 's/.*Shared library: \[\([^]]*\)\].*/\1/p' \
		| while IFS= read -r library; do
			[ -n "${library}" ] && copy_needed_runtime "${library}"
			done
fi

# Remove debug and linker-only sections from every ELF copied into the
# initramfs.  BusyBox is normally already stripped, while the sysroot glibc
# files often contain debug sections that would otherwise dominate the image.
if command -v "${STRIP}" >/dev/null 2>&1 && command -v file >/dev/null 2>&1; then
	find "${STAGING_ROOT}" -type f -print | while IFS= read -r candidate; do
		if file -b "${candidate}" | grep -q 'ELF'; then
			"${STRIP}" --strip-unneeded "${candidate}"
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
	"${STAGING_ROOT}/etc/init.d/rcK"
chmod 0700 "${STAGING_ROOT}/root"
chmod 1777 "${STAGING_ROOT}/tmp" "${STAGING_ROOT}/var/tmp"

# The early init path needs a console before devtmpfs is mounted. fakeroot
# supplies the device node metadata without requiring the build to run as root.
mknod -m 0600 "${STAGING_ROOT}/dev/console" c 5 1 2>/dev/null || true

if [ "$(id -u)" -eq 0 ]; then
	chown -hR 0:0 "${STAGING_ROOT}"
fi

mkdir -p "$(dirname "${OUTPUT_FILE}")"
temporary_output="${OUTPUT_FILE}.tmp"
rm -f "${temporary_output}"
(
	cd "${STAGING_ROOT}"
	find . -print | LC_ALL=C sort | cpio --quiet -o -H newc
) | gzip -9 > "${temporary_output}"
mv -f "${temporary_output}" "${OUTPUT_FILE}"

echo "Created ${OUTPUT_FILE}"
du -h "${OUTPUT_FILE}"
