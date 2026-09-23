#!/usr/bin/env bash
# Phytium DWMAC DKMS Driver One-Click Uninstaller
set -e

PKG_NAME="phytium-eth"
PKG_VER="1.0"
SRC_DIR="/usr/src/${PKG_NAME}-${PKG_VER}"

if [ "$EUID" -ne 0 ]; then
    echo "[-] Please run as root (sudo ./uninstall.sh)"
    exit 1
fi

echo "[*] Removing kernel module..."
if lsmod | grep -q "dwmac_phytium"; then
    rmmod dwmac_phytium || true
fi

echo "[*] Removing module from DKMS..."
dkms remove -m "${PKG_NAME}" -v "${PKG_VER}" --all || true

echo "[*] Cleaning up ${SRC_DIR}..."
rm -rf "${SRC_DIR}"

echo "[+] Uninstallation complete."
