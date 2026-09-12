#!/bin/sh

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
BASE_ROOTFS="${SCRIPT_DIR}/base_rootfs"
INIT_SCRIPTS="${SCRIPT_DIR}/init_scripts"
STAGING_ROOT="${SCRIPT_DIR}/rootfs"
IMAGE_FILE="${SCRIPT_DIR}/rootfs.ext2"
IMAGE_SIZE_MB=${ROOTFS_IMAGE_SIZE_MB:-32}

if [ ! -d "${BASE_ROOTFS}" ]; then
    echo "ERROR: base rootfs directory not found: ${BASE_ROOTFS}" >&2
    exit 1
fi

rm -rf "${STAGING_ROOT}"
mkdir -p "${STAGING_ROOT}"
cp -a "${BASE_ROOTFS}/." "${STAGING_ROOT}/"

# Empty runtime mount points are not preserved by Git, so create them for
# every image build.  The kernel needs /dev to exist before starting init
# when CONFIG_DEVTMPFS_MOUNT is enabled.
mkdir -p \
    "${STAGING_ROOT}/dev/pts" \
    "${STAGING_ROOT}/dev/shm" \
    "${STAGING_ROOT}/proc" \
    "${STAGING_ROOT}/sys" \
    "${STAGING_ROOT}/run" \
    "${STAGING_ROOT}/tmp" \
    "${STAGING_ROOT}/root" \
    "${STAGING_ROOT}/etc/init.d" \
    "${STAGING_ROOT}/var/log" \
    "${STAGING_ROOT}/var/tmp"

if [ ! -d "${INIT_SCRIPTS}" ]; then
    echo "ERROR: init scripts directory not found: ${INIT_SCRIPTS}" >&2
    exit 1
fi
cp -a "${INIT_SCRIPTS}/." "${STAGING_ROOT}/etc/init.d/"

chmod 0755 \
    "${STAGING_ROOT}/dev" \
    "${STAGING_ROOT}/dev/pts" \
    "${STAGING_ROOT}/dev/shm" \
    "${STAGING_ROOT}/proc" \
    "${STAGING_ROOT}/sys" \
    "${STAGING_ROOT}/run" \
    "${STAGING_ROOT}/root" \
    "${STAGING_ROOT}/etc/init.d" \
    "${STAGING_ROOT}/var" \
    "${STAGING_ROOT}/var/log"
chmod 0755 "${STAGING_ROOT}/etc/init.d/rcS" "${STAGING_ROOT}/etc/init.d/rcK"
chmod 0644 \
    "${STAGING_ROOT}/etc/group" \
    "${STAGING_ROOT}/etc/hostname" \
    "${STAGING_ROOT}/etc/hosts" \
    "${STAGING_ROOT}/etc/inittab" \
    "${STAGING_ROOT}/etc/nsswitch.conf" \
    "${STAGING_ROOT}/etc/passwd" \
    "${STAGING_ROOT}/etc/profile"
chmod 0700 "${STAGING_ROOT}/root"
chmod 1777 "${STAGING_ROOT}/tmp" "${STAGING_ROOT}/var/tmp"

ln -snf /proc/mounts "${STAGING_ROOT}/etc/mtab"
ln -snf /run "${STAGING_ROOT}/var/run"

# The top-level build invokes this script through fakeroot.  Normalize image
# ownership while still allowing a direct non-root diagnostic build.
if [ "$(id -u)" -eq 0 ]; then
    chown -hR 0:0 "${STAGING_ROOT}"
fi

rm -f "${IMAGE_FILE}"
truncate -s "${IMAGE_SIZE_MB}M" "${IMAGE_FILE}"
mkfs.ext2 -q -F -L rootfs -d "${STAGING_ROOT}" "${IMAGE_FILE}"
rm -rf "${STAGING_ROOT}"

echo "Created ${IMAGE_FILE} (${IMAGE_SIZE_MB} MiB)"
