# Maintainer: yb <fun2buu@gmail.com>
pkgname=phytium-dwmac-dkms-git
_pkgname=phytium-eth
pkgver=1.1.0
pkgrel=1
pkgdesc="DKMS driver for Phytium FTGM0001 GMAC (FTC663/FT2000/D2000) for Linux 6.x and 7.x"
arch=('aarch64')
url="https://github.com/bin2yx/phytium-dwmac-dkms"
license=('GPL2')
depends=('dkms')
makedepends=('git')
provides=("phytium-dwmac-dkms")
conflicts=("phytium-dwmac-dkms")
source=("git+https://github.com/bin2yx/phytium-dwmac-dkms.git")
sha256sums=('SKIP')

package() {
    cd "${srcdir}/phytium-dwmac-dkms"

    local destdir="${pkgdir}/usr/src/${_pkgname}-${pkgver}"
    install -dm755 "${destdir}"

    cp -rf common.h descs.h dkms.conf dwmac-phytium.c hwif.h Makefile mmc.h stmmac.h stmmac_platform.h "${destdir}/"

    # Update version in dkms.conf
    sed -i "s/PACKAGE_VERSION=\".*\"/PACKAGE_VERSION=\"${pkgver}\"/" "${destdir}/dkms.conf"
}
