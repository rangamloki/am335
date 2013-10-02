/*
 * Board support file for Phytec phyBOARD phyCORE Am335x Board.
 *
 * Copyright (C) 2013 Phytec Embedded Pvt. Ltd.
 *
 * Author: Trilokesh Rangam <lokesh.r@phytec.in>
 *
 * Based on mach-omap2/board-am335xevm.c
 *
 * Based on mach-omap2/board-pcm051.c
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
#include <linux/i2c/at24.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/ethtool.h>
#include <linux/mfd/tps65910.h>
#include <linux/reboot.h>
#include <linux/opp.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

#include <mach/hardware.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/hardware/asp.h>

#include <plat/omap_device.h>
#include <plat/irqs.h>
#include <plat/board.h>
#include <plat/common.h>
#include <plat/emif.h>
#include <plat/nand.h>
#include <plat/mmc.h>

#include "board-flash.h"
#include "cpuidle33xx.h"
#include "mux.h"
#include "hsmmc.h"
#include "devices.h"

#include "common.h"
#include "am33xx_generic.h"

/* Convert GPIO signal to GPIO pin number */
#define GPIO_TO_PIN(bank, gpio) (32 * (bank) + (gpio))

#define GPIO_RTC_PMIC_IRQ  GPIO_TO_PIN(3, 4)
#define GPIO_RTC_RV4162C7_IRQ  GPIO_TO_PIN(0, 20)

#define EEPROM_I2C_ADDR         0x52

#define PHYCORE_AM335_BASE(initfnc, base) strcat(base, "_");\
					strcat(base,initfnc);\
					int* (void) initfnc();

extern void am33xx_d_can_init(unsigned int instance);
static char phycore_am335_carrier[12] = "none";

static int __init phycore_am335_board_setup(char *str)
{
	if (str)
	strlcpy(phycore_am335_carrier, str, sizeof(phycore_am335_carrier));
	return 0;
}
__setup("board=", phycore_am335_board_setup);

/* module pin mux structure */
struct pinmux_config {
	const char *string_name; /* signal name format */
	int val; /* Options for the mux register value */
};

/*
* @pin_mux - single module pin-mux structure which defines pin-mux
*                       details for all its pins.
*/
static void setup_pin_mux(struct pinmux_config *pin_mux)
{
	int i;

	for (i = 0; pin_mux->string_name != NULL; pin_mux++)
		omap_mux_init_signal(pin_mux->string_name, pin_mux->val);

}

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

/* Module pin mux for mmc0 */
static struct pinmux_config pbac01_mmc0_pin_mux[] = {
	{"mmc0_dat3.mmc0_dat3", OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"mmc0_dat2.mmc0_dat2", OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"mmc0_dat1.mmc0_dat1", OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"mmc0_dat0.mmc0_dat0", OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"mmc0_clk.mmc0_clk",   OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"mmc0_cmd.mmc0_cmd",   OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"spi0_cs1.mmc0_sdcd",  OMAP_MUX_MODE7 | AM33XX_PIN_INPUT_PULLUP},
	{NULL, 0},
};

static struct pinmux_config pbac01_d_can1_pin_mux[] = {
	{"uart1_txd.dcan1_rx_mux2", OMAP_MUX_MODE2 | AM33XX_PIN_INPUT_PULLUP},
	{"uart1_rxd.dcan1_tx_mux2", OMAP_MUX_MODE2 | AM33XX_PULL_ENBL},
	{NULL, 0},
};

static struct omap2_hsmmc_info pbac01_mmc[] __initdata = {
	{
		.mmc            = 1,
		.caps           = MMC_CAP_4_BIT_DATA,
		.gpio_cd        = GPIO_TO_PIN(0, 6),
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

static void pbac01_mmc0_init(void)
{
        setup_pin_mux(pbac01_mmc0_pin_mux);

        omap2_hsmmc_init(pbac01_mmc);
        return;
}

static struct resource am33xx_cpuidle_resources[] = {
	{
		.start          = AM33XX_EMIF0_BASE,
		.end            = AM33XX_EMIF0_BASE + SZ_32K - 1,
		.flags          = IORESOURCE_MEM,
	},
};

/* AM33XX devices support DDR2 power down */
static struct am33xx_cpuidle_config am33xx_cpuidle_pdata = {
	.ddr2_pdown     = 1,
};

static struct platform_device am33xx_cpuidle_device = {
	.name                   = "cpuidle-am33xx",
	.num_resources          = ARRAY_SIZE(am33xx_cpuidle_resources),
	.resource               = am33xx_cpuidle_resources,
	.dev = {
		.platform_data  = &am33xx_cpuidle_pdata,
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

static struct at24_platform_data am335x_at24_eeprom_info = {
	.byte_len       = (32*1024) / 8,
	.page_size      = 32,
	.flags          = AT24_FLAG_ADDR16,
	.context        = (void *)NULL,
};

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

static struct tps65910_board am335x_tps65910_info = {
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
	.irq                            = OMAP_GPIO_IRQ(GPIO_RTC_PMIC_IRQ),
};

static void am335x_nand_init(void)
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

static void pbac01_d_can_init(void)
{
	/* Instance Zero */
	setup_pin_mux(pbac01_d_can1_pin_mux);
	am33xx_d_can_init(1);
}

static struct pinmux_config rtc_pin_mux[] = {
	{"xdma_event_intr1.gpio0_20", OMAP_MUX_MODE7 | AM33XX_PIN_INPUT_PULLUP},
	/* gpio0_20 is shared by lcd touch irq and rtc irq */
	{"mii1_rxdv.gpio3_4", OMAP_MUX_MODE7 | AM33XX_PIN_INPUT_PULLUP},
	{NULL, 0},
};

static void __init rtc_irq_init(void)
{
	int r;
	setup_pin_mux(rtc_pin_mux);

		/* Option 1: RV-4162 */
	r = gpio_request_one(GPIO_RTC_RV4162C7_IRQ,
				GPIOF_IN, "rtc-rv4162c7-irq");
	if (r < 0) {
		printk(KERN_WARNING "failed to request GPIO%d\n",
				GPIO_RTC_RV4162C7_IRQ);
		return;
	}

		/* Option 2: RTC in the TPS65910 PMIC */
	if (omap_mux_init_signal("mii1_rxdv.gpio3_4", AM33XX_PIN_INPUT_PULLUP))
		printk(KERN_WARNING "Failed to mux PMIC IRQ\n");
	else if (gpio_request_one(GPIO_RTC_PMIC_IRQ,
					GPIOF_IN, "rtc-tps65910-irq") < 0)
		printk(KERN_WARNING "failed to request GPIO%d\n",
					GPIO_RTC_PMIC_IRQ);
}

static struct i2c_board_info __initdata phycore_am335_i2c_boardinfo[] = {
	{
		I2C_BOARD_INFO("tps65910", TPS65910_I2C_ID1),
		.platform_data  = &am335x_tps65910_info,
	},
	{
		/* Baseboard board EEPROM */
		I2C_BOARD_INFO("24c32", EEPROM_I2C_ADDR),
		.platform_data  = &am335x_at24_eeprom_info,
	},
	{
		I2C_BOARD_INFO("rv4162c7", 0x68),
		.irq = OMAP_GPIO_IRQ(GPIO_RTC_RV4162C7_IRQ),
	},
};
static void __init phycore_am335_i2c_init(void)
{
	omap_register_i2c_bus(1, 100, phycore_am335_i2c_boardinfo,
				ARRAY_SIZE(phycore_am335_i2c_boardinfo));
}

static void __init phycore_am335_init(void)
{
	am33xx_cpuidle_init();
	am33xx_mux_init(NULL);
	omap_serial_init();
	omap_sdrc_init(NULL, NULL);
	am335x_nand_init();
	rtc_irq_init();
	phycore_am335_i2c_init();
//	PHYCORE_AM335_BASE(mmc0_init, *dummy);
	pbac01_mmc0_init();
	pbac01_d_can_init();
}

static void __init am335x_map_io(void)
{
	omap2_set_globals_am33xx();
	omapam33xx_map_common_io();
}

MACHINE_START(PHYCORE_AM335, "phyBOARD phyCORE AM335x")
	/* Maintainer: PHYTEC */
	.atag_offset    = 0x100,
	.map_io         = am335x_map_io,
	.init_early     = am33xx_init_early,
	.init_irq       = ti81xx_init_irq,
	.handle_irq     = omap3_intc_handle_irq,
	.timer          = &omap3_am33xx_timer,
	.init_machine   = phycore_am335_init,
MACHINE_END
