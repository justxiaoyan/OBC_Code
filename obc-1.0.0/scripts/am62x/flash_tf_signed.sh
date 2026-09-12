#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/../.." && pwd)
IMAGE_DIR="$REPO_ROOT/output/image"
PLATFORM_NAME="am62x"
TARGET_DEV=""
FLASH_SCOPE="all"
AUTO_YES=0
FORCE_SDA=0
OBC_HEADER_SIZE=512

# Partition offsets and sizes must match dts/am62x/k3-am625-sk.dts.
LOADER_PART_SIZE=0x00400000
FDT_PART_SIZE=0x00080000
TEEOS_PART_SIZE=0x00400000
UBOOT_PART_SIZE=0x00400000
KERNEL_PART_SIZE=0x02000000
ROOTFS_PART_SIZE=0x02000000

usage() {
    cat <<'USAGE'
Usage:
  flash_tf_signed.sh <target_dev> [all|loader|fdt|teeos|uboot|kernel|rootfs] [--image-dir <dir>] [--yes] [--force-sda]

Examples:
  sudo ./flash_tf_signed.sh /dev/sdb all
  sudo ./flash_tf_signed.sh /dev/mmcblk0 fdt --yes
  sudo ./flash_tf_signed.sh /dev/sdc uboot --image-dir /path/to/output/image --yes
  sudo ./flash_tf_signed.sh /dev/sda all --force-sda

Signed artifact names expected in image-dir:
  am62x-loader.bin
  am62x-fdt.bin
  am62x-teeos.bin
  am62x-uboot.bin
  am62x-kernel.bin
  am62x-rootfs.bin

The 512-byte OBCFS header is removed while writing loader and rootfs.
The header is retained for fdt, teeos, uboot and kernel.

FDT source used by the build: dts/am62x/k3-am625-sk.dts
USAGE
}

check_scope() {
    case "$1" in
        all|loader|fdt|teeos|uboot|kernel|rootfs) ;;
        *)
            printf 'Error: invalid scope "%s"\n' "$1"
            usage
            exit 1
            ;;
    esac
}

require_file() {
    local file="$1"
    if [[ ! -f "$file" ]]; then
        printf 'Error: file not found: %s\n' "$file"
        exit 1
    fi
}

read_le_u32() {
    local file="$1"
    local offset="$2"
    local -a bytes

    read -r -a bytes < <(od -An -v -tu1 -j "$offset" -N 4 "$file")
    if [[ ${#bytes[@]} -ne 4 ]]; then
        return 1
    fi
    printf '%u' "$((bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24)))"
}

read_le_u16() {
    local file="$1"
    local offset="$2"
    local -a bytes

    read -r -a bytes < <(od -An -v -tu1 -j "$offset" -N 2 "$file")
    if [[ ${#bytes[@]} -ne 2 ]]; then
        return 1
    fi
    printf '%u' "$((bytes[0] | (bytes[1] << 8)))"
}

validate_image() {
    local image_file="$1"
    local expected_head_write="$2"
    local partition_size_hex="$3"
    local total_size
    local payload_size
    local head_write
    local partition_size=$((partition_size_hex))
    local magic

    require_file "$image_file"
    total_size=$(stat -c '%s' "$image_file")
    if ((total_size <= OBC_HEADER_SIZE)); then
        printf 'Error: image is too small to contain an OBCFS header: %s\n' "$image_file"
        exit 1
    fi

    magic=$(LC_ALL=C head -c 5 "$image_file")
    if [[ "$magic" != "OBCFS" ]]; then
        printf 'Error: invalid OBCFS header in %s\n' "$image_file"
        exit 1
    fi

    payload_size=$(read_le_u32 "$image_file" 6) || {
        printf 'Error: cannot read payload size from %s\n' "$image_file"
        exit 1
    }
    head_write=$(read_le_u16 "$image_file" 12) || {
        printf 'Error: cannot read head_write_flag from %s\n' "$image_file"
        exit 1
    }

    if ((total_size != OBC_HEADER_SIZE + payload_size)); then
        printf 'Error: package size mismatch in %s (header=%d, actual=%d)\n' \
            "$image_file" "$payload_size" "$((total_size - OBC_HEADER_SIZE))"
        exit 1
    fi
    if ((head_write != expected_head_write)); then
        printf 'Error: unexpected head_write_flag in %s (expected=%d, actual=%d)\n' \
            "$image_file" "$expected_head_write" "$head_write"
        exit 1
    fi

    if ((expected_head_write == 0)); then
        VALIDATED_WRITE_SIZE=$payload_size
    else
        VALIDATED_WRITE_SIZE=$total_size
    fi

    if ((VALIDATED_WRITE_SIZE > partition_size)); then
        printf 'Error: %s write size %d exceeds partition size %d\n' \
            "$image_file" "$VALIDATED_WRITE_SIZE" "$partition_size"
        exit 1
    fi
}

write_one() {
    local image_file="$1"
    local part_name="$2"
    local offset_hex="$3"
    local partition_size_hex="$4"
    local expected_head_write="$5"

    local offset_dec=$((offset_hex))
    local -a dd_args
    local header_note=""

    validate_image "$image_file" "$expected_head_write" "$partition_size_hex"

    dd_args=(
        "if=$image_file"
        "of=$TARGET_DEV"
        "bs=4M"
        "seek=$offset_dec"
        "oflag=seek_bytes"
        "conv=notrunc,fsync"
        "status=progress"
    )
    if ((expected_head_write == 0)); then
        dd_args+=("skip=$OBC_HEADER_SIZE" "iflag=skip_bytes")
        header_note=", OBCFS header removed"
    fi

    printf '\n[FLASH] %s -> %s @ %s (%d bytes%s)\n' \
        "$(basename "$image_file")" "$part_name" "$offset_hex" \
        "$VALIDATED_WRITE_SIZE" "$header_note"
    sudo dd "${dd_args[@]}"
}

flash_loader() {
    local img="$IMAGE_DIR/${PLATFORM_NAME}-loader.bin"
    write_one "$img" loader0 0x00000000 "$LOADER_PART_SIZE" 0
    write_one "$img" loader1 0x00400000 "$LOADER_PART_SIZE" 0
}

flash_fdt() {
    local img="$IMAGE_DIR/${PLATFORM_NAME}-fdt.bin"
    write_one "$img" fdt0 0x00900000 "$FDT_PART_SIZE" 1
    write_one "$img" fdt1 0x00980000 "$FDT_PART_SIZE" 1
    write_one "$img" fdt2 0x00A00000 "$FDT_PART_SIZE" 1
}

flash_teeos() {
    local img="$IMAGE_DIR/${PLATFORM_NAME}-teeos.bin"
    write_one "$img" teeos0 0x00A80000 "$TEEOS_PART_SIZE" 1
    write_one "$img" teeos1 0x00E80000 "$TEEOS_PART_SIZE" 1
    write_one "$img" teeos2 0x01280000 "$TEEOS_PART_SIZE" 1
}

flash_uboot() {
    local img="$IMAGE_DIR/${PLATFORM_NAME}-uboot.bin"
    write_one "$img" uboot0 0x01680000 "$UBOOT_PART_SIZE" 1
    write_one "$img" uboot1 0x01A80000 "$UBOOT_PART_SIZE" 1
    write_one "$img" uboot2 0x01E80000 "$UBOOT_PART_SIZE" 1
}

flash_kernel() {
    local img="$IMAGE_DIR/${PLATFORM_NAME}-kernel.bin"
    write_one "$img" kernel0 0x02280000 "$KERNEL_PART_SIZE" 1
    write_one "$img" kernel1 0x04280000 "$KERNEL_PART_SIZE" 1
    write_one "$img" kernel2 0x06280000 "$KERNEL_PART_SIZE" 1
}

flash_rootfs() {
    local img="$IMAGE_DIR/${PLATFORM_NAME}-rootfs.bin"
    write_one "$img" rootfs0 0x08280000 "$ROOTFS_PART_SIZE" 0
    write_one "$img" rootfs1 0x0A280000 "$ROOTFS_PART_SIZE" 0
    write_one "$img" rootfs2 0x0C280000 "$ROOTFS_PART_SIZE" 0
}

preflight_scope() {
    case "$FLASH_SCOPE" in
        all|loader)
            validate_image "$IMAGE_DIR/${PLATFORM_NAME}-loader.bin" 0 "$LOADER_PART_SIZE"
            ;;
    esac
    case "$FLASH_SCOPE" in
        all|fdt)
            validate_image "$IMAGE_DIR/${PLATFORM_NAME}-fdt.bin" 1 "$FDT_PART_SIZE"
            ;;
    esac
    case "$FLASH_SCOPE" in
        all|teeos)
            validate_image "$IMAGE_DIR/${PLATFORM_NAME}-teeos.bin" 1 "$TEEOS_PART_SIZE"
            ;;
    esac
    case "$FLASH_SCOPE" in
        all|uboot)
            validate_image "$IMAGE_DIR/${PLATFORM_NAME}-uboot.bin" 1 "$UBOOT_PART_SIZE"
            ;;
    esac
    case "$FLASH_SCOPE" in
        all|kernel)
            validate_image "$IMAGE_DIR/${PLATFORM_NAME}-kernel.bin" 1 "$KERNEL_PART_SIZE"
            ;;
    esac
    case "$FLASH_SCOPE" in
        all|rootfs)
            validate_image "$IMAGE_DIR/${PLATFORM_NAME}-rootfs.bin" 0 "$ROOTFS_PART_SIZE"
            ;;
    esac
}

required_device_size() {
    case "$FLASH_SCOPE" in
        loader) printf '%d' "$((0x00800000))" ;;
        fdt)    printf '%d' "$((0x00A80000))" ;;
        teeos)  printf '%d' "$((0x01680000))" ;;
        uboot)  printf '%d' "$((0x02280000))" ;;
        kernel) printf '%d' "$((0x08280000))" ;;
        rootfs|all) printf '%d' "$((0x0E280000))" ;;
    esac
}

main() {
    if [[ $# -eq 1 && ( "$1" == "-h" || "$1" == "--help" ) ]]; then
        usage
        exit 0
    fi
    if [[ $# -lt 1 ]]; then
        usage
        exit 1
    fi

    TARGET_DEV="$1"
    shift

    if [[ $# -gt 0 && "$1" != --* ]]; then
        FLASH_SCOPE="$1"
        shift
    fi
    check_scope "$FLASH_SCOPE"

    while [[ $# -gt 0 ]]; do
        case "$1" in
            --image-dir)
                if [[ $# -lt 2 ]]; then
                    printf 'Error: --image-dir requires a directory argument\n'
                    exit 1
                fi
                IMAGE_DIR="$2"
                shift 2
                ;;
            --yes)
                AUTO_YES=1
                shift
                ;;
            --force-sda)
                FORCE_SDA=1
                shift
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            *)
                printf 'Error: unknown option "%s"\n' "$1"
                usage
                exit 1
                ;;
        esac
    done

    if [[ "$TARGET_DEV" == "/dev/sda" && $FORCE_SDA -ne 1 ]]; then
        printf 'Error: refuse to write /dev/sda for safety (use --force-sda if this is really your TF card).\n'
        exit 1
    fi
    if [[ ! -b "$TARGET_DEV" ]]; then
        printf 'Error: target device is not a block device: %s\n' "$TARGET_DEV"
        exit 1
    fi
    if [[ ! -d "$IMAGE_DIR" ]]; then
        printf 'Error: image directory not found: %s\n' "$IMAGE_DIR"
        exit 1
    fi

    local dev_type
    local dev_size
    local min_size
    local mounted

    dev_type=$(lsblk -ndo TYPE "$TARGET_DEV" | head -n 1)
    if [[ "$dev_type" == "part" ]]; then
        printf 'Error: target must be the whole TF device, not a partition: %s\n' "$TARGET_DEV"
        exit 1
    fi

    mounted=$(lsblk -nrpo NAME,MOUNTPOINT "$TARGET_DEV" | awk 'NF > 1')
    if [[ -n "$mounted" ]]; then
        printf 'Error: target device has mounted filesystems; unmount them first:\n%s\n' "$mounted"
        exit 1
    fi

    dev_size=$(lsblk -bndo SIZE "$TARGET_DEV" | head -n 1)
    min_size=$(required_device_size)
    if [[ -z "$dev_size" ]] || ((dev_size < min_size)); then
        printf 'Error: target device is too small (need at least %d bytes)\n' "$min_size"
        exit 1
    fi

    preflight_scope

    printf 'Target device: %s\n' "$TARGET_DEV"
    printf 'Scope:         %s\n' "$FLASH_SCOPE"
    printf 'Image dir:     %s\n' "$IMAGE_DIR"

    if [[ "$TARGET_DEV" == "/dev/sda" ]]; then
        printf 'WARNING: writing to /dev/sda (forced).\n'
    fi

    if [[ $AUTO_YES -ne 1 ]]; then
        printf 'This will overwrite raw regions on %s. Continue? (y/N): ' "$TARGET_DEV"
        read -r answer
        if [[ "$answer" != "y" && "$answer" != "Y" ]]; then
            printf 'Cancelled.\n'
            exit 0
        fi
    fi

    case "$FLASH_SCOPE" in
        all)
            flash_loader
            flash_fdt
            flash_teeos
            flash_uboot
            flash_kernel
            flash_rootfs
            ;;
        loader) flash_loader ;;
        fdt)    flash_fdt ;;
        teeos)  flash_teeos ;;
        uboot)  flash_uboot ;;
        kernel) flash_kernel ;;
        rootfs) flash_rootfs ;;
    esac

    sync
    printf '\nDone: flash completed successfully.\n'
}

main "$@"
