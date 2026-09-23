#!/usr/bin/env bash
# Phytium DWMAC DKMS Driver One-Click Installer
set -e

PKG_NAME="phytium-eth"
PKG_VER="1.0"
SRC_DIR="/usr/src/${PKG_NAME}-${PKG_VER}"

if [ "$EUID" -ne 0 ]; then
    echo "[-] Please run as root (sudo ./install.sh)"
    exit 1
fi

echo "[*] Installing dependencies check..."
if ! command -v dkms >/dev/null 2>&1; then
    echo "[-] DKMS not found. Installing via pacman..."
    pacman -S --noconfirm --needed dkms
fi

KVER=$(uname -r)
if [ ! -d "/lib/modules/${KVER}/build" ]; then
    echo "[-] Kernel headers not found for ${KVER}."
    echo "    Please install linux-headers (e.g., sudo pacman -S linux-headers)."
    exit 1
fi

echo "[*] Preparing DKMS source directory at ${SRC_DIR}..."
mkdir -p "${SRC_DIR}"
cp -rf common.h descs.h dkms.conf dwmac-phytium.c hwif.h Makefile mmc.h stmmac.h stmmac_platform.h "${SRC_DIR}/"

echo "[*] Registering and building via DKMS..."
if dkms status -m "${PKG_NAME}" -v "${PKG_VER}" | grep -q "${PKG_NAME}"; then
    echo "[*] Existing DKMS module found. Removing old version..."
    dkms remove -m "${PKG_NAME}" -v "${PKG_VER}" --all || true
fi

dkms add -m "${PKG_NAME}" -v "${PKG_VER}"
dkms build -m "${PKG_NAME}" -v "${PKG_VER}"
dkms install -m "${PKG_NAME}" -v "${PKG_VER}" --force

echo "[*] Enabling dkms.service for automatic rebuilds on kernel updates..."
systemctl enable dkms.service 2>/dev/null || true

echo "[*] Reloading kernel module..."
if lsmod | grep -q "dwmac_phytium"; then
    rmmod dwmac_phytium || true
fi
modprobe dwmac_phytium

echo "[*] Optimizing ARP settings to prevent multi-homing ARP flux..."
mkdir -p /etc/sysctl.d
cat << 'SYSCTL_EOF' > /etc/sysctl.d/99-phytium-network.conf
net.ipv4.conf.all.arp_ignore = 1
net.ipv4.conf.all.arp_announce = 2
net.ipv4.conf.default.arp_ignore = 1
net.ipv4.conf.default.arp_announce = 2
SYSCTL_EOF
sysctl -p /etc/sysctl.d/99-phytium-network.conf >/dev/null 2>&1 || true

echo "[+] Installation complete! Verifying network link..."
ip link show enaftgm1i0 2>/dev/null || ip link show eth0 2>/dev/null || true

echo ""
echo "[!] IMPORTANT: If your system uses initramfs, consider running:"
echo "    sudo mkinitcpio -P"
echo "    to ensure the updated driver is bundled into the boot image."
