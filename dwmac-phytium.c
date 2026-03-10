// SPDX-License-Identifier: GPL-2.0-only
#include <linux/acpi.h>
#include <linux/clk-provider.h>
#include <linux/clkdev.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include "stmmac.h"
#include "stmmac_platform.h"

static int dwmac_phytium_get_resources(struct platform_device *pdev,
				struct stmmac_resources *stmmac_res)
{
	memset(stmmac_res, 0, sizeof(*stmmac_res));

	stmmac_res->irq = platform_get_irq(pdev, 0);
	if (stmmac_res->irq < 0)
		return stmmac_res->irq;

	stmmac_res->addr = devm_platform_ioremap_resource(pdev, 0);
	stmmac_res->wol_irq = stmmac_res->irq;
	stmmac_res->lpi_irq = -ENOENT;

	return PTR_ERR_OR_ZERO(stmmac_res->addr);
}

static struct plat_stmmacenet_data *
dwmac_phytium_parse_config_acpi(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct fwnode_handle *np = dev_fwnode(dev);
	struct plat_stmmacenet_data *plat;
	struct stmmac_dma_cfg *dma_cfg;
	struct stmmac_axi *axi;
	struct clk_hw *clk_hw;
	u64 clk_freq;
	int ret;
	char clk_name[32];

	plat = devm_kzalloc(dev, sizeof(*plat), GFP_KERNEL);
	if (!plat)
		return ERR_PTR(-ENOMEM);

	plat->mdio_bus_data = devm_kzalloc(dev, sizeof(*plat->mdio_bus_data), GFP_KERNEL);
	if (!plat->mdio_bus_data)
		return ERR_PTR(-ENOMEM);

	plat->phy_interface = PHY_INTERFACE_MODE_RGMII_ID;
	
	/* 终极真理：这其实是一张老款的 GMAC 网卡！
	 * 纠正后，内核才能在正确的内存偏移地址找到 MAC 和 MDIO 控制器 */
	plat->core_type = DWMAC_CORE_GMAC;

	/* 开启真正的时钟自动计算逻辑，适配 GMAC 架构 */
	plat->clk_csr = -1;

	if (fwnode_property_read_u32(np, "max-speed", &plat->max_speed))
		plat->max_speed = 1000;

	plat->bus_id = 1; 
	plat->phy_addr = -1;

	plat->maxmtu = JUMBO_LEN;
	fwnode_property_read_u32(np, "max-frame-size", &plat->maxmtu);
	fwnode_property_read_u32(np, "snps,ps-speed", &plat->mac_port_sel_speed);

	dma_cfg = devm_kzalloc(dev, sizeof(*dma_cfg), GFP_KERNEL);
	if (!dma_cfg)
		return ERR_PTR(-ENOMEM);
	plat->dma_cfg = dma_cfg;

	dma_cfg->pbl = 16;
	dma_cfg->txpbl = 16;
	dma_cfg->rxpbl = 16;
	dma_cfg->pblx8 = !fwnode_property_read_bool(np, "snps,no-pbl-x8");
	dma_cfg->fixed_burst = true;
	dma_cfg->aal = fwnode_property_read_bool(np, "snps,aal");

	plat->force_sf_dma_mode = fwnode_property_read_bool(np, "snps,force_sf_dma_mode");
	plat->force_thresh_dma_mode = fwnode_property_read_bool(np, "snps,force_thresh_dma_mode");
	if (plat->force_thresh_dma_mode)
		plat->force_sf_dma_mode = 0;

	axi = devm_kzalloc(dev, sizeof(*axi), GFP_KERNEL);
	if (!axi)
		return ERR_PTR(-ENOMEM);
	plat->axi = axi;
	axi->axi_wr_osr_lmt = 1;
	axi->axi_rd_osr_lmt = 1;
	axi->axi_fb = true;

	plat->rx_queues_to_use = 1;
	plat->tx_queues_to_use = 1;
	plat->rx_queues_cfg[0].mode_to_use = MTL_QUEUE_DCB;
	plat->tx_queues_cfg[0].mode_to_use = MTL_QUEUE_DCB;
	plat->rx_queues_cfg[0].use_prio = true;
	plat->rx_sched_algorithm = MTL_RX_ALGORITHM_SP;
	plat->tx_sched_algorithm = MTL_TX_ALGORITHM_SP;

	ret = fwnode_property_read_u64(np, "clock-frequency", &clk_freq);
	if (ret < 0)
		clk_freq = 250000000;

	snprintf(clk_name, sizeof(clk_name), "%s_clk", dev_name(dev));
	clk_hw = devm_clk_hw_register_fixed_rate(dev, clk_name, NULL, 0, clk_freq);
	if (IS_ERR(clk_hw))
		return ERR_CAST(clk_hw);

	devm_clk_hw_register_clkdev(dev, clk_hw, NULL, dev_name(dev));
	plat->stmmac_clk = clk_hw->clk;
	plat->pclk = clk_hw->clk; 

	ret = clk_prepare_enable(plat->stmmac_clk);
	if (ret)
		return ERR_PTR(ret);

	return plat;
}

static int dwmac_phytium_probe(struct platform_device *pdev)
{
	struct plat_stmmacenet_data *plat_dat;
	struct stmmac_resources stmmac_res;
	unsigned long long uid;
	acpi_status status;
	int ret;

	if (has_acpi_companion(&pdev->dev)) {
		status = acpi_evaluate_integer(ACPI_HANDLE(&pdev->dev), "_UID", NULL, &uid);
		if (ACPI_SUCCESS(status) && uid != 0) {
			dev_info(&pdev->dev, "Custom Rule: Skipping secondary MAC (UID %lld)\n", uid);
			return -ENODEV; 
		}
	}

	ret = dwmac_phytium_get_resources(pdev, &stmmac_res);
	if (ret)
		return ret;

	plat_dat = dwmac_phytium_parse_config_acpi(pdev);
	if (IS_ERR(plat_dat))
		return PTR_ERR(plat_dat);

	usleep_range(10000, 15000);

	ret = stmmac_dvr_probe(&pdev->dev, plat_dat, &stmmac_res);
	if (ret && plat_dat->stmmac_clk)
		clk_disable_unprepare(plat_dat->stmmac_clk);

	return ret;
}

static const struct acpi_device_id dwmac_phytium_acpi_match[] = {
	{ "FTGM0001" },
	{}
};
MODULE_DEVICE_TABLE(acpi, dwmac_phytium_acpi_match);

static struct platform_driver dwmac_phytium_driver = {
	.probe  = dwmac_phytium_probe,
	.remove = stmmac_pltfr_remove,
	.driver = {
		.name           = "dwmac-phytium",
		.pm		= &stmmac_pltfr_pm_ops,
		.acpi_match_table = ACPI_PTR(dwmac_phytium_acpi_match),
	},
};
module_platform_driver(dwmac_phytium_driver);

MODULE_DESCRIPTION("Phytium DWMAC Driver - Restored GMAC architecture for 6.19 API");
MODULE_LICENSE("GPL v2");
