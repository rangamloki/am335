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
#include <linux/phy.h>
#include <linux/module.h>
#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/export.h>
#include <linux/ethtool.h>
#include <linux/mfd/tps65910.h>
#include <linux/pwm_backlight.h>
#include <linux/input/edt-ft5x06.h>
#include <linux/reboot.h>
#include <linux/opp.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/micrel_phy.h>
#include <linux/pwm/pwm.h>

#include <video/da8xx-fb.h>

#include <mach/hardware.h>
#include <mach/board-phycore-am335.h>

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
#include <plat/usb.h>
#include <plat/lcdc.h>

#include "board-flash.h"
#include "cpuidle33xx.h"
#include "mux.h"
#include "hsmmc.h"
#include "devices.h"

#include "common.h"
#include "am33xx_generic.h"

/* Convert GPIO signal to GPIO pin number */
#define GPIO_TO_PIN(bank, gpio) (32 * (bank) + (gpio))

#if defined(CONFIG_TOUCHSCREEN_EDT_FT5X06) || \
		defined(CONFIG_TOUCHSCREEN_EDT_FT5X06_MODULE)
#define AM335X_PHYCORE_EDT_FT5X06_GPIO_IRQ GPIO_TO_PIN(3, 19)
#endif

#define GPIO_RTC_PMIC_IRQ  GPIO_TO_PIN(3, 4)
#define GPIO_RTC_RV4162C7_IRQ  GPIO_TO_PIN(0, 20)

#define EEPROM_I2C_ADDR         0x52

/* LCD backlight platform Data */
#define AM335X_BACKLIGHT_MAX_BRIGHTNESS		100
#define AM335X_BACKLIGHT_DEFAULT_BRIGHTNESS	0
#define AM335X_PWM_PERIOD_NANO_SECONDS		100000

#define PWM_DEVICE_ID	"ecap.0"

static char phycore_carrier_str[CB_STR_LEN] __initdata = "none";

static int __init phycore_am335_board_setup(char *str)
{
	int i;
	if (str) {
		strlcpy(phycore_carrier_str, str, sizeof(phycore_carrier_str));
		for (i = 0; i < ARRAY_SIZE(list_boards); i++) {
			if (strcmp(phycore_carrier_str,
						list_boards[i].name) == 0)
				valid_brd_name = 1;
		}
	}
	if (!valid_brd_name)
		printk(KERN_WARNING "WARNING! No valid carrier Board found !!!\n");

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

/* Module pin mux for rmii1 */
static struct pinmux_config rmii1_pin_mux[] = {
	{"mii1_crs.rmii1_crs_dv", OMAP_MUX_MODE1 | AM33XX_PIN_INPUT_PULLDOWN},
	{"mii1_rxerr.mii1_rxerr", OMAP_MUX_MODE1 | AM33XX_PIN_INPUT_PULLDOWN},
	{"mii1_txen.mii1_txen", OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT},
	{"mii1_txd1.mii1_txd1", OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT},
	{"mii1_txd0.mii1_txd0", OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT},
	{"mii1_rxd1.mii1_rxd1", OMAP_MUX_MODE1 | AM33XX_PIN_INPUT_PULLDOWN},
	{"mii1_rxd0.mii1_rxd0", OMAP_MUX_MODE1 | AM33XX_PIN_INPUT_PULLDOWN},
	{"rmii1_refclk.rmii1_refclk", OMAP_MUX_MODE0 |
					AM33XX_PIN_INPUT_PULLDOWN},
	{"mdio_data.mdio_data", OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"mdio_clk.mdio_clk", OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT_PULLUP},
	{NULL, 0},
};

/* Module pin mux for rgmii2 */
static struct pinmux_config rgmii2_pin_mux[] = {
	{"gpmc_a0.rgmii2_tctl", OMAP_MUX_MODE2 | AM33XX_PIN_OUTPUT},
	{"gpmc_a1.rgmii2_rctl", OMAP_MUX_MODE2 | AM33XX_PIN_INPUT_PULLDOWN},
	{"gpmc_a2.rgmii2_td3", OMAP_MUX_MODE2 | AM33XX_PIN_OUTPUT},
	{"gpmc_a3.rgmii2_td2", OMAP_MUX_MODE2 | AM33XX_PIN_OUTPUT},
	{"gpmc_a4.rgmii2_td1", OMAP_MUX_MODE2 | AM33XX_PIN_OUTPUT},
	{"gpmc_a5.rgmii2_td0", OMAP_MUX_MODE2 | AM33XX_PIN_OUTPUT},
	{"gpmc_a6.rgmii2_tclk", OMAP_MUX_MODE2 | AM33XX_PIN_OUTPUT},
	{"gpmc_a7.rgmii2_rclk", OMAP_MUX_MODE2 | AM33XX_PIN_INPUT_PULLDOWN},
	{"gpmc_a8.rgmii2_rd3", OMAP_MUX_MODE2 | AM33XX_PIN_INPUT_PULLDOWN},
	{"gpmc_a9.rgmii2_rd2", OMAP_MUX_MODE2 | AM33XX_PIN_INPUT_PULLDOWN},
	{"gpmc_a10.rgmii2_rd1", OMAP_MUX_MODE2 | AM33XX_PIN_INPUT_PULLDOWN},
	{"gpmc_a11.rgmii2_rd0", OMAP_MUX_MODE2 | AM33XX_PIN_INPUT_PULLDOWN},
	{"mdio_data.mdio_data", OMAP_MUX_MODE0 | AM33XX_PIN_INPUT_PULLUP},
	{"mdio_clk.mdio_clk", OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT_PULLUP},
	{NULL, 0},
};

/* Module pin mux for LCDC */
static struct pinmux_config lcdc_pin_mux[] = {
	{"lcd_data0.lcd_data0",         OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
							| AM33XX_PULL_DISA},
	{"lcd_data1.lcd_data1",         OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
							| AM33XX_PULL_DISA},
	{"lcd_data2.lcd_data2",         OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
							| AM33XX_PULL_DISA},
	{"lcd_data3.lcd_data3",         OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
							| AM33XX_PULL_DISA},
	{"lcd_data4.lcd_data4",         OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
							| AM33XX_PULL_DISA},
	{"lcd_data5.lcd_data5",         OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
							| AM33XX_PULL_DISA},
	{"lcd_data6.lcd_data6",         OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
							| AM33XX_PULL_DISA},
	{"lcd_data7.lcd_data7",         OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
							| AM33XX_PULL_DISA},
	{"lcd_data8.lcd_data8",         OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
							| AM33XX_PULL_DISA},
	{"lcd_data9.lcd_data9",         OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
							| AM33XX_PULL_DISA},
	{"lcd_data10.lcd_data10",       OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
							| AM33XX_PULL_DISA},
	{"lcd_data11.lcd_data11",       OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
							| AM33XX_PULL_DISA},
	{"lcd_data12.lcd_data12",       OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
							| AM33XX_PULL_DISA},
	{"lcd_data13.lcd_data13",       OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
							| AM33XX_PULL_DISA},
	{"lcd_data14.lcd_data14",       OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
							| AM33XX_PULL_DISA},
	{"lcd_data15.lcd_data15",       OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT
							| AM33XX_PULL_DISA},
	{"gpmc_ad8.lcd_data16",         OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT},
	{"gpmc_ad9.lcd_data17",         OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT},
	{"gpmc_ad10.lcd_data18",        OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT},
	{"gpmc_ad11.lcd_data19",        OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT},
	{"gpmc_ad12.lcd_data20",        OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT},
	{"gpmc_ad13.lcd_data21",        OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT},
	{"gpmc_ad14.lcd_data22",        OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT},
	{"gpmc_ad15.lcd_data23",        OMAP_MUX_MODE1 | AM33XX_PIN_OUTPUT},
	{"lcd_vsync.lcd_vsync",         OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT},
	{"lcd_hsync.lcd_hsync",         OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT},
	{"lcd_pclk.lcd_pclk",           OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT},
	{"lcd_ac_bias_en.lcd_ac_bias_en", OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT},
	{NULL, 0},
};

/* pinmux for usb0 drvvbus */
static struct pinmux_config usb0_pin_mux[] = {
	{"usb0_drvvbus.usb0_drvvbus",   OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT},
	{"mcasp0_ahclkr.gpio3_17",      OMAP_MUX_MODE7 | AM33XX_PULL_ENBL |
						AM33XX_PIN_INPUT_PULLUP},
	{NULL, 0},
};

/* pinmux for usb1 drvvbus */
static struct pinmux_config usb1_pin_mux[] = {
	{"usb1_drvvbus.usb1_drvvbus",   OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT},
	{"mcasp0_aclkr.gpio3_18",       OMAP_MUX_MODE7 | AM33XX_PULL_ENBL |
						AM33XX_PIN_INPUT_PULLUP},
	{NULL, 0},
};

static struct pinmux_config ecap0_pin_mux[] = {
	{"ecap0_in_pwm0_out.ecap0_in_pwm0_out",
			OMAP_MUX_MODE0 | AM33XX_PIN_OUTPUT},
	{NULL, 0},
};

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

static int pbac01_ksz9021_phy_fixup(struct phy_device *phydev)
{
	/* Set KSZ9021_CLKSKEW */
	phy_write(phydev, KSZ9021_EXTCTRL, (KSZ9021_WRBIT | KSZ9021_CLKSKEW));
	phy_write(phydev, KSZ9021_EXTWR, KSZ9021_CLKSKEW_VAL);

	phy_write(phydev, KSZ9021_EXTCTRL, KSZ9021_CLKSKEW);
	printk(KERN_INFO "CLKSKEW == 0x%x\n", (phy_read(phydev,
						KSZ9021_EXTRD)));

	/* Set KSZ9021_RXSKEW */
	phy_write(phydev, KSZ9021_EXTCTRL, (KSZ9021_WRBIT | KSZ9021_RXSKEW));
	phy_write(phydev, KSZ9021_EXTWR, KSZ9021_RXSKEW_VAL);

	phy_write(phydev, KSZ9021_EXTCTRL, KSZ9021_RXSKEW);
	printk(KERN_INFO "RXSKEW == 0x%x\n", (phy_read(phydev, KSZ9021_EXTRD)));

	/* Set KSZ9021_TXSKEW */
	phy_write(phydev, KSZ9021_EXTCTRL, (KSZ9021_WRBIT | KSZ9021_TXSKEW));
	phy_write(phydev, KSZ9021_EXTWR, KSZ9021_TXSKEW_VAL);

	phy_write(phydev, KSZ9021_EXTCTRL, KSZ9021_TXSKEW);
	printk(KERN_INFO "TXSKEW == 0x%x\n", (phy_read(phydev, KSZ9021_EXTRD)));

	return 0;
}

static struct omap_musb_board_data musb_board_data = {
	.interface_type = MUSB_INTERFACE_ULPI,
	/*
	* mode[0:3] = USB0PORT's mode
	* mode[4:7] = USB1PORT's mode
	* PCM051 has USB0 in OTG mode and USB1 in host mode.
	*/
	.mode           = (MUSB_HOST << 4) | MUSB_OTG,
	.power          = 500,
	.instances      = 1,
};

static struct platform_pwm_backlight_data am335x_backlight_data = {
	.pwm_id         = PWM_DEVICE_ID,
	.ch             = -1,
	.max_brightness = AM335X_BACKLIGHT_MAX_BRIGHTNESS,
	.dft_brightness = AM335X_BACKLIGHT_DEFAULT_BRIGHTNESS,
	.pwm_period_ns  = AM335X_PWM_PERIOD_NANO_SECONDS,
};

/* Setup pwm-backlight */
static struct platform_device am335x_backlight = {
	.name           = "pwm-backlight",
	.id             = -1,
	.dev            = {
		.platform_data  = &am335x_backlight_data,
	}
};

static struct pwmss_platform_data pwm_pdata = {
	.version = PWM_VERSION_1
};

static const struct display_panel disp_panel = {
	WVGA,
	32,
	32,
	COLOR_ACTIVE,
};

static struct lcd_ctrl_config lcd_cfg = {
	&disp_panel,
	.ac_bias                = 40,
	.ac_bias_intrpt         = 0,
	.dma_burst_sz           = 16,
	.bpp                    = 32,
	.fdd                    = 0x80,
	.tft_alt_mode           = 0,
	.stn_565_mode           = 0,
	.mono_8bit_mode         = 0,
	.invert_line_clock      = 1,
	.invert_frm_clock       = 1,
	.sync_edge              = 0,
	.sync_ctrl              = 1,
	.raster_order           = 0,
};

static struct da8xx_lcdc_platform_data lcdc_pdata[] = {
	{
		.manu_name              = "Emerging",
		.controller_data        = &lcd_cfg,
		.type                   = "ETM0700G0DH6",
	},
	{
		.manu_name              = "PrimeView",
		.controller_data        = &lcd_cfg,
		.type                   = "PV_PM070WL4",
	}, {
		.manu_name              = "PrimeView",
		.controller_data        = &lcd_cfg,
		.type                   = "PV_PD035VL1",
	}, {
		.manu_name              = "PrimeView",
		.controller_data        = &lcd_cfg,
		.type                   = "PV_PD050VL1",
	}, {
		.manu_name              = "PrimeView",
		.controller_data        = &lcd_cfg,
		.type                   = "PV_PD104SLF",
	}
};

static struct da8xx_lcdc_selection_platform_data lcdc_selection_pdata = {
	.entries_ptr = lcdc_pdata,
	.entries_cnt = ARRAY_SIZE(lcdc_pdata)
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

static void pbac01_mmc0_init(void)
{
	setup_pin_mux(pbac01_mmc0_pin_mux);

	omap2_hsmmc_init(pbac01_mmc);
	return;
}

static void rmii1_init(void)
{
	setup_pin_mux(rmii1_pin_mux);
	return;
}

static void rgmii2_init(void)
{
	setup_pin_mux(rgmii2_pin_mux);
	return;
}

static void usb_init(void)
{
	setup_pin_mux(usb0_pin_mux);
	setup_pin_mux(usb1_pin_mux);
	usb_musb_init(&musb_board_data);
	return;
}

static int __init ecap0_init(void)
{
	setup_pin_mux(ecap0_pin_mux);
	am33xx_register_ecap(0, &pwm_pdata);
	platform_device_register(&am335x_backlight);

	return 0;
}
late_initcall(ecap0_init);

static int __init conf_disp_pll(int rate)
{
	struct clk *disp_pll;
	int ret = -EINVAL;

	disp_pll = clk_get(NULL, "dpll_disp_ck");
	if (IS_ERR(disp_pll)) {
		pr_err("Cannot clk_get disp_pll\n");
		goto out;
	}

	ret = clk_set_rate(disp_pll, rate);
	clk_put(disp_pll);
out:
	return ret;
}

static void lcdc_init(void)
{
	setup_pin_mux(lcdc_pin_mux);

	if (conf_disp_pll(300000000)) {
		pr_info("Failed configure display PLL, not attempting to"
				"register LCDC\n");
	return;
	}

	if (am33xx_register_lcdc(&lcdc_selection_pdata))
		pr_info("Failed to register LCDC device\n");
	return;
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

static void pbac01_ethernet_init(void)
{
	rmii1_init();
	rgmii2_init();
	am33xx_cpsw_init(AM33XX_CPSW_MODE_RMII1_RGMII2, NULL, NULL);
	phy_register_fixup_for_uid(PHY_ID_KSZ9021, 0x000ffffe,
					pbac01_ksz9021_phy_fixup);
}

#if defined(CONFIG_TOUCHSCREEN_EDT_FT5X06) || \
		defined(CONFIG_TOUCHSCREEN_EDT_FT5X06_MODULE)
static struct edt_ft5x06_platform_data pba_ft5x06_data = {
	.reset_pin      = -1,           /* static high */
	.irq_pin        = AM335X_PHYCORE_EDT_FT5X06_GPIO_IRQ,
};
#endif

static struct i2c_board_info __initdata phycore_am335_i2c_boardinfo[] = {
#if defined(CONFIG_TOUCHSCREEN_EDT_FT5X06) || \
		defined(CONFIG_TOUCHSCREEN_EDT_FT5X06_MODULE)
	{
		I2C_BOARD_INFO("edt-ft5x06", 0x38),
		.irq = OMAP_GPIO_IRQ(AM335X_PHYCORE_EDT_FT5X06_GPIO_IRQ),
		.platform_data = &pba_ft5x06_data,
	},
#endif
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

static struct phycore_am335_carrier list_devices[] = {
	{ "pba-c-01", pbac01_mmc0_init },
	{ "pba-c-01", pbac01_ethernet_init },
	{"null", NULL},
};
static void __init phycore_am335_init(void)
{
	int i;
	am33xx_cpuidle_init();
	am33xx_mux_init(NULL);
	omap_serial_init();
	omap_sdrc_init(NULL, NULL);
	am335x_nand_init();
	rtc_irq_init();
	phycore_am335_i2c_init();
	usb_init();
	lcdc_init();

	for (i = 0; i < ARRAY_SIZE(list_devices); i++) {
		if (strcmp(phycore_carrier_str,
				list_devices[i].board_name) == 0)
			list_devices[i].devinit();
	}
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
