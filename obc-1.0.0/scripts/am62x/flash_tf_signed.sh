#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
REPO_ROOT=$(cd "$SCRIPT_DIR/../.." && pwd)
IMAGE_DIR="$REPO_ROOT/output/image"
TARGET_DEV=""
FLASH_SCOPE="all"
AUTO_YES=0
FORCE_SDA=0

usage() {
    cat <<'USAGE'
Usage:
  flash_tf_signed.sh <target_dev> [all|loader|fdt|teeos|uboot] [--image-dir <dir>] [--yes] [--force-sda]

Examples:
  sudo ./flash_tf_signed.sh /dev/sdb all
  sudo ./flash_tf_signed.sh /dev/mmcblk0 fdt --yes
  sudo ./flash_tf_signed.sh /dev/sdc uboot --image-dir /path/to/output/image --yes
  sudo ./flash_tf_signed.sh /dev/sda all --force-sda

Signed artifact names expected in image-dir:
  100p-loader.bin
  100p-fdt.bin
  100p-teeos.bin
  100p-uboot.bin

FDT source used by the build: dts/am62x/k3-am625-sk.dts
USAGE
}

check_scope() {
    case "$1" in
        all|loader|fdt|teeos|uboot) ;;
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

write_one() {
    local image_file="$1"
    local part_name="$2"
    local offset_hex="$3"

    local offset_dec=$((offset_hex))
    local seek_k=$((offset_dec / 1024))

    printf '\n[FLASH] %s -> %s @ %s\n' "$(basename "$image_file")" "$part_name" "$offset_hex"
    sudo dd if="$image_file" of="$TARGET_DEV" bs=1K seek="$seek_k" conv=fsync status=progress
}

flash_loader() {
    local img="$IMAGE_DIR/100p-loader.bin"
    require_file "$img"
    write_one "$img" loader0 0x00000000
    write_one "$img" loader1 0x00400000
}

flash_fdt() {
    local img="$IMAGE_DIR/100p-fdt.bin"
    require_file "$img"
    write_one "$img" fdt0 0x00900000
    write_one "$img" fdt1 0x00980000
    write_one "$img" fdt2 0x00A00000
}

flash_teeos() {
    local img="$IMAGE_DIR/100p-teeos.bin"
    require_file "$img"
    write_one "$img" teeos0 0x00A80000
    write_one "$img" teeos1 0x00E80000
    write_one "$img" teeos2 0x01280000
}

flash_uboot() {
    local img="$IMAGE_DIR/100p-uboot.bin"
    require_file "$img"
    write_one "$img" uboot0 0x01680000
    write_one "$img" uboot1 0x01A80000
    write_one "$img" uboot2 0x01E80000
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
            ;;
        loader) flash_loader ;;
        fdt)    flash_fdt ;;
        teeos)  flash_teeos ;;
        uboot)  flash_uboot ;;
    esac

    sync
    printf '\nDone: flash completed successfully.\n'
}

main "$@"
