Phytium DWMAC Ethernet Driver (DKMS)

这是专为国产飞腾平台（飞腾 FTC663 / FT-2000/4 / D2000 / 长城等整机）适配现代 Linux（如 Arch Linux ARM，内核 6.x 和 7.x+）的千兆有线网卡驱动。

主要修复的问题：
1. 架构识别：修复内核升级后将千兆 GMAC 误识别为百兆老网卡、随机生成 MAC 地址以及 MDIO 注册失败的问题。
2. 内核 ABI 兼容：自动适配新内核（Linux 6.19+ 及 7.x）中断结构体偏移变更，解决启动时申请中断失败（error -22）。
3. 千兆丢包根治：采用 RGMII_RXID 模式，消除 1000M 高频下 MAC 与 PHY 双重发送延时叠加冲突，实现千兆线速 0 丢包、0 延迟卡顿。
4. 电源时钟优化：修复休眠挂起时时钟双重释放报警。

一键安装：
git clone https://github.com/bin2yx/phytium-dwmac-dkms.git
cd phytium-dwmac-dkms
sudo ./install.sh

一键卸载：
sudo ./uninstall.sh

Arch Linux 用户也可以直接打包安装：
makepkg -si

注意：如果系统启用了引导镜像（initramfs），安装后建议执行一次更新镜像，防止开机从旧镜像加载旧驱动：
sudo mkinitcpio -P
