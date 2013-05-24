/*
 * Some AM335X generic board code.
 *
 * Copyright (C) 2013 Teresa Gámez Zerban <t.gamez@phytec.de>
 *
 * Based on AM335x EVM:
 * Copyright (C) 2011 Texas Instruments, Inc. - http://www.ti.com/
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/rtc/rtc-omap.h>
#include <mach/hardware.h>
#include <plat/omap_hwmod.h>
#include <plat/omap_device.h>
#include <asm/io.h>

void __iomem *am33xx_emif_base;

void __iomem * __init am33xx_get_mem_ctlr(void)
{
	am33xx_emif_base = ioremap(AM33XX_EMIF0_BASE, SZ_32K);

	if (!am33xx_emif_base)
		pr_warning("%s: Unable to map DDR3 controller", __func__);

	return am33xx_emif_base;
}

void __iomem *am33xx_get_ram_base(void)
{
	return am33xx_emif_base;
}

void __iomem *am33xx_gpio0_base;

void __iomem *am33xx_get_gpio0_base(void)
{
	am33xx_gpio0_base = ioremap(AM33XX_GPIO0_BASE, SZ_4K);

	return am33xx_gpio0_base;
}

/* unlock values for rtc */
#define RTC_UNLOCK_VAL1			0x83e70b13
#define RTC_UNLOCK_VAL2			0x95a4f1e0

/* offset addr for unlock and clock control reg */
#define RTC_KICK0R_OFFSET		0x6c
#define RTC_KICK1R_OFFSET		0x70
#define RTC_OSC_REG_OFFSET		0x54

/* clock control reg bits */
#define RTC_32K_CLK_SEL_EX		(0x1 << 3)
#define RTC_32K_CLK_EN			(0x1 << 6)

static struct omap_rtc_pdata am335x_rtc_info = {
	.pm_off		= false,
	.wakeup_capable	= 0,
};

void am335x_internal_rtc_init(void)
{
	void __iomem *base;
	struct clk *clk;
	struct omap_hwmod *oh;
	struct platform_device *pdev;
	char *dev_name = "am33xx-rtc";

	clk = clk_get(NULL, "rtc_fck");
	if (IS_ERR(clk)) {
		pr_err("rtc : Failed to get RTC clock\n");
		return;
	}

	if (clk_enable(clk)) {
		pr_err("rtc: Clock Enable Failed\n");
		return;
	}

	base = ioremap(AM33XX_RTC_BASE, SZ_4K);

	if (WARN_ON(!base))
		return;

	/* Unlock the rtc's registers */
	writel(RTC_UNLOCK_VAL1, base + RTC_KICK0R_OFFSET);
	writel(RTC_UNLOCK_VAL2, base + RTC_KICK1R_OFFSET);

	/*
	 * Enable the 32K OSc
	 * TODO: Need a better way to handle this
	 * we need to do it before the rtc probe happens
	 */
	writel((RTC_32K_CLK_SEL_EX | RTC_32K_CLK_EN),\
			base + RTC_OSC_REG_OFFSET);

	iounmap(base);

	clk_disable(clk);
	clk_put(clk);

	if (omap_rev() == AM335X_REV_ES2_0)
		am335x_rtc_info.wakeup_capable = 1;

	oh = omap_hwmod_lookup("rtc");
	if (!oh) {
		pr_err("could not look up %s\n", "rtc");
		return;
	}

	pdev = omap_device_build(dev_name, -1, oh, &am335x_rtc_info,
			sizeof(struct omap_rtc_pdata), NULL, 0, 0);
	WARN(IS_ERR(pdev), "Can't build omap_device for %s:%s.\n",
			dev_name, oh->name);
}
