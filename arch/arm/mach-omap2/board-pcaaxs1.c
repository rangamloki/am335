/*
 * Board support file for Phytec phyCARD-AM335x Board.
 *
 * Copyright (C) 2013 Phytec Embedded Pvt. Ltd.
 *
 * Author: Trilokesh Rangam <lokesh.r@phytec.in>
 *
 * Based on mach-omap2/board-am335xevm.c
 *
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
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/mfd/tps65910.h>
#include <linux/mfd/tps65217.h>
#include <linux/reboot.h>
#include <linux/opp.h>

#include <mach/hardware.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/hardware/asp.h>

#include <plat/omap_device.h>
#include <plat/irqs.h>
#include <plat/board.h>
#include <plat/common.h>
#include <plat/mmc.h>
#include <plat/emif.h>
#include <plat/nand.h>

#include "board-flash.h"
#include "cpuidle33xx.h"
#include "mux.h"
#include "hsmmc.h"
#include "devices.h"

/* Convert GPIO signal to GPIO pin number */
#define GPIO_TO_PIN(bank, gpio) (32 * (bank) + (gpio))
#include "common.h"

#define GPIO_RTC_PMIC_IRQ  GPIO_TO_PIN(1, 19)

static struct omap2_hsmmc_info am335x_mmc[] __initdata = {
	{
		.mmc            = 1,
		.caps           = MMC_CAP_4_BIT_DATA,
		.gpio_cd        = GPIO_TO_PIN(0, 31),
		.gpio_wp        = -EINVAL,
		.ocr_mask       = MMC_VDD_32_33 | MMC_VDD_33_34, /* 3V3 */
	},
	{
		.mmc            = 0,    /* will be set at runtime */
	},
	{
		.mmc            = 0,    /* will be set at runtime */
	},
	{}      /* Terminator */
};

/* module pin mux structure */
struct pinmux_config {
	const char *string_name; /* signal name format */
	int val; /* Options for the mux register value */
};

/*
* @pin_mux - single module pin-mux structure which defines pin-mux
*			details for all its pins.
*/
static void setup_pin_mux(struct pinmux_config *pin_mux)
{
	int i;

	for (i = 0; pin_mux->string_name != NULL; pin_mux++)
		omap_mux_init_signal(pin_mux->string_name, pin_mux->val);

}

/* Module pin mux for mmc0 */
static struct pinmux_config mmc0_pin_mux[] = {
	{"mmc0_dat3.mmc0_dat3",	OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"mmc0_dat2.mmc0_dat2",	OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"mmc0_dat1.mmc0_dat1",	OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"mmc0_dat0.mmc0_dat0",	OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"mmc0_clk.mmc0_clk",	OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"mmc0_cmd.mmc0_cmd",	OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_wpn.gpio0_31",	OMAP_MUX_MODE7 | AM33XX_PIN_INPUT_PULLUP},
	{NULL, 0},
};

/* Module pin mux for i2c0 */
static struct pinmux_config i2c0_pin_mux[] = {
	{"i2c0_sda.i2c0_sda", OMAP_MUX_MODE0 | AM33XX_SLEWCTRL_SLOW |
				AM33XX_INPUT_EN | AM33XX_PIN_OUTPUT},
	{"i2c0_scl.i2c0_scl", OMAP_MUX_MODE0 | AM33XX_SLEWCTRL_SLOW |
				AM33XX_INPUT_EN | AM33XX_PIN_OUTPUT},
	{NULL, 0},
};

/* Module pin mux for i2c1 */
static struct pinmux_config i2c1_pin_mux[] = {
	{"uart1_rxd.i2c1_sda", OMAP_MUX_MODE3 | AM33XX_SLEWCTRL_SLOW |
					AM33XX_PIN_INPUT_PULLUP},
	{"uart1_txd.i2c1_scl", OMAP_MUX_MODE3 | AM33XX_SLEWCTRL_SLOW |
					AM33XX_PIN_INPUT_PULLUP},
	{NULL, 0},
};

static struct pinmux_config rtc_pin_mux[] = {
	{"gpmc_a3.gpio1_19", OMAP_MUX_MODE7 | AM33XX_PIN_INPUT_PULLUP},
	{NULL, 0},
};

/* Pin mux for nand flash module */
static struct pinmux_config nand_pin_mux[] = {
	{"gpmc_ad0.gpmc_ad0",     OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad1.gpmc_ad1",     OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad2.gpmc_ad2",     OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad3.gpmc_ad3",     OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad4.gpmc_ad4",     OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad5.gpmc_ad5",     OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad6.gpmc_ad6",     OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_ad7.gpmc_ad7",     OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_wait0.gpmc_wait0", OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_wpn.gpmc_wpn",     OMAP_MUX_MODE7 | AM33XX_PIN_INPUT_PULLUP},
	{"gpmc_csn0.gpmc_csn0",   OMAP_MUX_MODE0 | AM33XX_PULL_DISA},
	{"gpmc_advn_ale.gpmc_advn_ale",  OMAP_MUX_MODE0 | AM33XX_PULL_DISA},
	{"gpmc_oen_ren.gpmc_oen_ren",    OMAP_MUX_MODE0 | AM33XX_PULL_DISA},
	{"gpmc_wen.gpmc_wen",     OMAP_MUX_MODE0 | AM33XX_PULL_DISA},
	{"gpmc_ben0_cle.gpmc_ben0_cle",  OMAP_MUX_MODE0 | AM33XX_PULL_DISA},
	{NULL, 0},
};

static struct gpmc_timings am335x_nand_timings = {

/* granularity of 10 is sufficient because of calculations */
	.sync_clk = 0,

	.cs_on = 0,
	.cs_rd_off = 30,
	.cs_wr_off = 30,

	.adv_on = 0,
	.adv_rd_off = 30,
	.adv_wr_off = 30,

	.oe_on = 10,
	.we_off = 20,
	.oe_off = 30,

	.access = 30,
	.rd_cycle = 30,
	.wr_cycle = 30,

	.cs_cycle_delay = 50,
	.cs_delay_en = 1,
	.wr_access = 30,
	.wr_data_mux_bus = 0,
};

static void pcaaxs1_nand_init(void)
{
	struct omap_nand_platform_data *pdata;
	struct gpmc_devices_info gpmc_device[2] = {
		{ NULL, 0 },
		{ NULL, 0 },
	};

	setup_pin_mux(nand_pin_mux);
	pdata = omap_nand_init(NULL, 0, 0, 0, &am335x_nand_timings);
	if (!pdata)
		return;
	pdata->ecc_opt = OMAP_ECC_BCH8_CODE_HW;
	pdata->elm_used = true;
	gpmc_device[0].pdata = pdata;
	gpmc_device[0].flag = GPMC_DEVICE_NAND;

	omap_init_gpmc(gpmc_device, sizeof(gpmc_device));
	omap_init_elm();
}

static struct regulator_init_data am335x_dummy = {
	.constraints.always_on  = true,
};

static struct regulator_consumer_supply am335x_vdd1_supply[] = {
	REGULATOR_SUPPLY("vdd_mpu", NULL),
};

static struct regulator_init_data am335x_vdd1 = {
	.constraints = {
		.min_uV                 = 600000,
		.max_uV                 = 1500000,
		.valid_modes_mask       = REGULATOR_MODE_NORMAL,
		.valid_ops_mask         = REGULATOR_CHANGE_VOLTAGE,
		.always_on              = 1,
	},
	.num_consumer_supplies  = ARRAY_SIZE(am335x_vdd1_supply),
	.consumer_supplies      = am335x_vdd1_supply,
};

struct tps65910_board am335x_tps65910_info = {
	.tps65910_pmic_init_data[TPS65910_REG_VRTC]     = &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VIO]      = &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VDD1]     = &am335x_vdd1,
	.tps65910_pmic_init_data[TPS65910_REG_VDD2]     = &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VDD3]     = &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VDIG1]    = &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VDIG2]    = &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VPLL]     = &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VDAC]     = &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VAUX1]    = &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VAUX2]    = &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VAUX33]   = &am335x_dummy,
	.tps65910_pmic_init_data[TPS65910_REG_VMMC]     = &am335x_dummy,
};

static void __init tps65910_rtc_irq_init(void)
{

	setup_pin_mux(rtc_pin_mux);

	/* RTC in the TPS65910 PMIC */
	if (omap_mux_init_signal("gpmc_a3.gpio1_19", AM33XX_PIN_INPUT_PULLUP))
		printk(KERN_WARNING "Failed to mux PMIC IRQ\n");
	else if (gpio_request_one(GPIO_RTC_PMIC_IRQ,
			GPIOF_IN, "rtc-tps65910-irq") < 0)
		printk(KERN_WARNING "failed to request GPIO%d\n",
			GPIO_RTC_PMIC_IRQ);
	else
		am335x_tps65910_info.irq = gpio_to_irq(GPIO_RTC_PMIC_IRQ);
}

static struct i2c_board_info __initdata pcaaxs1_i2c0_boardinfo[] = {
	{
		I2C_BOARD_INFO("tps65910", TPS65910_I2C_ID1),
		.platform_data  = &am335x_tps65910_info,
	},
};
static struct i2c_board_info __initdata pcaaxs1_i2c1_boardinfo[] = {
	{},
};

static void __init pcaaxs1_i2c_init(void)
{
	setup_pin_mux(i2c0_pin_mux);
	omap_register_i2c_bus(1, 100, pcaaxs1_i2c0_boardinfo,
				ARRAY_SIZE(pcaaxs1_i2c0_boardinfo));
	setup_pin_mux(i2c1_pin_mux);
	omap_register_i2c_bus(2, 100, pcaaxs1_i2c1_boardinfo,
				ARRAY_SIZE(pcaaxs1_i2c1_boardinfo));
}

/* Enable clkout1 */
static struct pinmux_config clkout1_pin_mux[] = {
	{"xdma_event_intr0.clkout1", OMAP_MUX_MODE3 | AM33XX_PIN_OUTPUT},
	{NULL, 0},
};

static void __init clkout1_enable(void)
{
	struct clk *ck_32;

	ck_32 = clk_get(NULL, "clkout1_ck");
	if (IS_ERR(ck_32)) {
		pr_err("Cannot clk_get ck_32\n");
		return;
	}

	clk_enable(ck_32);

	setup_pin_mux(clkout1_pin_mux);
}

void __iomem *am33xx_emif_base;

void __iomem * __init am33xx_get_mem_ctlr(void)
{

	am33xx_emif_base = ioremap(AM33XX_EMIF0_BASE, SZ_32K);

	if (!am33xx_emif_base)
		pr_warning("%s: Unable to map DDR3 controller",	__func__);

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

static void mmc0_init(void)
{
	setup_pin_mux(mmc0_pin_mux);

	omap2_hsmmc_init(am335x_mmc);
	return;
}

static struct resource am33xx_cpuidle_resources[] = {
	{
		.start		= AM33XX_EMIF0_BASE,
		.end		= AM33XX_EMIF0_BASE + SZ_32K - 1,
		.flags		= IORESOURCE_MEM,
	},
};

/* AM33XX devices support DDR2 power down */
static struct am33xx_cpuidle_config am33xx_cpuidle_pdata = {
	.ddr2_pdown	= 1,
};

static struct platform_device am33xx_cpuidle_device = {
	.name			= "cpuidle-am33xx",
	.num_resources		= ARRAY_SIZE(am33xx_cpuidle_resources),
	.resource		= am33xx_cpuidle_resources,
	.dev = {
		.platform_data	= &am33xx_cpuidle_pdata,
	},
};

static void __init am33xx_cpuidle_init(void)
{
	int ret;

	am33xx_cpuidle_pdata.emif_base = am33xx_get_mem_ctlr();

	ret = platform_device_register(&am33xx_cpuidle_device);

	if (ret)
		pr_warning("AM33XX cpuidle registration failed\n");

}

static void __init pcaaxs1_init(void)
{
	am33xx_cpuidle_init();
	am33xx_mux_init(NULL);
	omap_serial_init();
	clkout1_enable();
	tps65910_rtc_irq_init();
	pcaaxs1_i2c_init();
	omap_sdrc_init(NULL, NULL);
	/* Create an alias for icss clock */
	if (clk_add_alias("pruss", NULL, "pruss_uart_gclk", NULL))
		pr_warn("failed to create an alias: pruss_uart_gclk --> pruss\n");
	/* Create an alias for gfx/sgx clock */
	if (clk_add_alias("sgx_ck", NULL, "gfx_fclk", NULL))
		pr_warn("failed to create an alias: gfx_fclk --> sgx_ck\n");
	mmc0_init();
	pcaaxs1_nand_init();
}

static void __init pcaaxs1_map_io(void)
{
	omap2_set_globals_am33xx();
	omapam33xx_map_common_io();
}

MACHINE_START(PCAAXS1, "pcaaxs1")
	/* Maintainer: PHYTEC */
	.atag_offset	= 0x100,
	.map_io		= pcaaxs1_map_io,
	.init_early	= am33xx_init_early,
	.init_irq	= ti81xx_init_irq,
	.handle_irq     = omap3_intc_handle_irq,
	.timer		= &omap3_am33xx_timer,
	.init_machine	= pcaaxs1_init,
MACHINE_END
