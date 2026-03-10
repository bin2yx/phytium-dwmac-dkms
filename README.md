# Phytium DWMAC Ethernet Driver (DKMS) for Linux 6.19+

[中文说明请见下半部分](#中文说明)

A DKMS-enabled Linux kernel driver for the **Phytium FTGM0001 Gigabit Ethernet MAC**, specifically patched to fix compatibility issues with Linux kernel **6.19 and newer** (e.g., on rolling-release distros like Arch Linux).

## 🐛 The Problem
When upgrading to Linux kernel 6.19+, the upstream `stmmac` network driver framework underwent significant API changes (e.g., deprecation of `has_gmac` flags in favor of `core_type`). 
Using the legacy out-of-tree driver on new kernels results in:
- The MAC being incorrectly identified as a legacy `DWMAC100` instead of a Gigabit MAC.
- Randomly generating MAC addresses on every boot due to reading incorrect memory offsets.
- Fatal `error -EIO: Cannot register the MDIO bus` and PHY probe failures.
- DMA capability overflows (`Rx/Tx FIFO size exceeds dma capability`).

## 🛠️ The Solution
This patched driver resolves these issues by:
1. Correcting the `core_type` to `DWMAC_CORE_GMAC` to match the actual Synopsys hardware architecture (ID: 0x36), fixing memory offset reading for the MAC address and MDIO controller.
2. Binding the `pclk` to prevent hardware registers from reading as `0x0`.
3. Setting `clk_csr = -1` to trigger modern kernel auto-calculation for MDIO clock dividers, preventing PHY communication timeouts.
4. Removing hardcoded FIFO depth limits to allow the kernel to auto-negotiate capabilities.
5. Adding an ACPI `_UID` filter to skip disconnected secondary MAC interfaces.

## 🚀 Installation (DKMS)

Ensure you have `dkms` and your kernel headers installed (e.g., `linux-headers` on Arch).

```bash
# 1. Clone the repository
git clone [https://github.com/bin2yx/phytium-dwmac-dkms.git](https://github.com/bin2yx/phytium-dwmac-dkms.git)
cd phytium-dwmac-dkms

# 2. Copy to DKMS source directory
sudo mkdir -p /usr/src/phytium-eth-1.0
sudo cp * /usr/src/phytium-eth-1.0/

# 3. Add, build, and install via DKMS
sudo dkms add -m phytium-eth -v 1.0
sudo dkms build -m phytium-eth -v 1.0
sudo dkms install -m phytium-eth -v 1.0

# 4. Enable DKMS service for automatic rebuilds on kernel updates
sudo systemctl enable dkms.service
