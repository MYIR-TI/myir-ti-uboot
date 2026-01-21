// SPDX-License-Identifier: GPL-2.0+
/*
 * Board specific initialization for AM62x platforms
 *
 * Copyright (C) 2020-2022 Texas Instruments Incorporated - https://www.ti.com/
 *	Suman Anna <s-anna@ti.com>
 *
 */

#include <efi_loader.h>
#include <env.h>
#include <spl.h>
#include <init.h>
#include <video.h>
#include <splash.h>
#include <cpu_func.h>
#include <k3-ddrss.h>
#include <fdt_support.h>
#include <fdt_simplefb.h>
#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <dm/uclass.h>
#include <asm/arch/k3-ddr.h>
#include <net.h>
#include <asm/gpio.h>
#include <cpu_func.h>
#include <dm/device.h>
#include <power/pmic.h>
#include <power/regulator.h>
#include <power/tps65219.h>
#include <i2c.h>
#include <linux/delay.h>
#include <linux/errno.h>

#include "../common/board_detect.h"
#include "../common/fdt_ops.h"

#include "../common/k3-ddr.h"

#define board_is_am62x_skevm()  (board_ti_k3_is("AM62-SKEVM") || \
				 board_ti_k3_is("AM62B-SKEVM"))
#define board_is_am62b_p1_skevm() board_ti_k3_is("AM62B-SKEVM-P1")
#define board_is_am62x_lp_skevm()  board_ti_k3_is("AM62-LP-SKEVM")
#define board_is_am62x_sip_skevm()  board_ti_k3_is("AM62SIP-SKEVM")
#define board_is_am62x_play()	board_ti_k3_is("BEAGLEPLAY-A0-")

DECLARE_GLOBAL_DATA_PTR;

#if CONFIG_IS_ENABLED(SPLASH_SCREEN)
static struct splash_location default_splash_locations[] = {
	{
		.name = "sf",
		.storage = SPLASH_STORAGE_SF,
		.flags = SPLASH_STORAGE_RAW,
		.offset = 0x700000,
	},
	{
		.name		= "mmc",
		.storage	= SPLASH_STORAGE_MMC,
		.flags		= SPLASH_STORAGE_FS,
		.devpart	= "1:1",
	},
};

int splash_screen_prepare(void)
{
	return splash_source_load(default_splash_locations,
				ARRAY_SIZE(default_splash_locations));
}
#endif

struct efi_fw_image fw_images[] = {
	{
		.image_type_id = AM62X_SK_TIBOOT3_IMAGE_GUID,
		.fw_name = u"AM62X_SK_TIBOOT3",
		.image_index = 1,
	},
	{
		.image_type_id = AM62X_SK_SPL_IMAGE_GUID,
		.fw_name = u"AM62X_SK_SPL",
		.image_index = 2,
	},
	{
		.image_type_id = AM62X_SK_UBOOT_IMAGE_GUID,
		.fw_name = u"AM62X_SK_UBOOT",
		.image_index = 3,
	}
};

struct efi_capsule_update_info update_info = {
	.dfu_string = "sf 0:0=tiboot3.bin raw 0 80000;"
	"tispl.bin raw 80000 200000;u-boot.img raw 280000 400000",
	.num_images = ARRAY_SIZE(fw_images),
	.images = fw_images,
};

#if IS_ENABLED(CONFIG_SET_DFU_ALT_INFO)
void set_dfu_alt_info(char *interface, char *devstr)
{
	if (IS_ENABLED(CONFIG_EFI_HAVE_CAPSULE_SUPPORT))
		env_set("dfu_alt_info", update_info.dfu_string);
}
#endif

int board_init(void)
{
	return 0;
}

#if CONFIG_IS_ENABLED(TI_I2C_BOARD_DETECT)
int do_board_detect(void)
{
	int ret;

	ret = ti_i2c_eeprom_am6_get_base(CONFIG_EEPROM_BUS_ADDRESS,
					 CONFIG_EEPROM_CHIP_ADDRESS);
	if (ret) {
		printf("EEPROM not available at 0x%02x, trying to read at 0x%02x\n",
		       CONFIG_EEPROM_CHIP_ADDRESS, CONFIG_EEPROM_CHIP_ADDRESS + 1);
		ret = ti_i2c_eeprom_am6_get_base(CONFIG_EEPROM_BUS_ADDRESS,
						 CONFIG_EEPROM_CHIP_ADDRESS + 1);
		if (ret)
			pr_err("Reading on-board EEPROM at 0x%02x failed %d\n",
			       CONFIG_EEPROM_CHIP_ADDRESS + 1, ret);
	}

	return ret;
}

int checkboard(void)
{
	struct ti_am6_eeprom *ep = TI_AM6_EEPROM_DATA;

	if (!do_board_detect())
		printf("Board: %s rev %s\n", ep->name, ep->version);

	return 0;
}

#if CONFIG_IS_ENABLED(BOARD_LATE_INIT)
static void setup_board_eeprom_env(void)
{
	char *name = "am62x_skevm";

	if (do_board_detect())
		goto invalid_eeprom;

	if (board_is_am62x_skevm())
		name = "am62x_skevm";
	else if (board_is_am62b_p1_skevm())
		name = "am62b_p1_skevm";
	else if (board_is_am62x_lp_skevm())
		name = "am62x_lp_skevm";
	else if (board_is_am62x_sip_skevm())
		name = "am62x_sip_skevm";
	else if (board_is_am62x_play())
		name = "am62x_beagleplay";
	else
		printf("Unidentified board claims %s in eeprom header\n",
		       board_ti_get_name());

invalid_eeprom:
	set_board_info_env_am6(name);
}

static void setup_serial(void)
{
	struct ti_am6_eeprom *ep = TI_AM6_EEPROM_DATA;
	unsigned long board_serial;
	char *endp;
	char serial_string[17] = { 0 };

	if (env_get("serial#"))
		return;

	board_serial = simple_strtoul(ep->serial, &endp, 16);
	if (*endp != '\0') {
		pr_err("Error: Can't set serial# to %s\n", ep->serial);
		return;
	}

	snprintf(serial_string, sizeof(serial_string), "%016lx", board_serial);
	env_set("serial#", serial_string);
}
#endif
#endif

#ifdef CONFIG_PMIC_TPS65219
int do_pmic_init(void)
{
        struct udevice *dev;
        int ret;

        u8 buck1;

        ret = uclass_get_device_by_driver(UCLASS_PMIC,DM_DRIVER_GET(pmic_tps65219), &dev);
        if (ret){
                /* No PMIC on board */
                return -1;
        }

        ret = pmic_reg_write(dev,TPS65219_BUCK1_VOUT_REG, 0x8a);
        if (ret){
                printf("failed to write tps65219...\n");
                return -1;
        }
        buck1 = pmic_reg_read(dev, TPS65219_BUCK1_VOUT_REG);
        if(buck1 == 0x8a){
                printf("BUCK1: vdd_core is 0.85v\n");
        }
        else if(buck1 == 0x86)
        {
                printf("BUCK1: vdd_core is 0.75v\n");
        }

        return 0;
}
#endif

/* NCA9555 I2C GPIO Expander definitions */
#define NCA9555_I2C_BUS		2	/* I2C bus number, adjust if needed */
#define NCA9555_I2C_ADDR	0x20	/* I2C address (0100 A2 A1 A0), adjust A0/A1/A2 if needed */

/* NCA9555 Register addresses */
#define NCA9555_REG_INPUT_PORT0	0x00
#define NCA9555_REG_INPUT_PORT1	0x01
#define NCA9555_REG_OUTPUT_PORT0	0x02
#define NCA9555_REG_OUTPUT_PORT1	0x03
#define NCA9555_REG_POLARITY_INV0	0x04
#define NCA9555_REG_POLARITY_INV1	0x05
#define NCA9555_REG_CONFIG_PORT0	0x06
#define NCA9555_REG_CONFIG_PORT1	0x07

/* IO0_2 is bit 2 of Port 0 */
#define NCA9555_IO0_2_BIT	(1 << 2)

/**
 * nca9555_reset_io0_2() - Reset device via NCA9555 IO0_2 pin
 *
 * This function performs a reset sequence on IO0_2:
 * 1. Configure IO0_2 as output
 * 2. Pull IO0_2 low
 * 3. Wait for reset pulse
 * 4. Pull IO0_2 high
 *
 * Return: 0 on success, negative on error
 */
static int nca9555_reset_io0_2(void)
{
	struct udevice *bus, *dev;
	u8 config_val, output_val;
	int ret, bus_num;
	u8 test_addr;

	/* Get I2C bus */
	ret = uclass_get_device_by_seq(UCLASS_I2C, NCA9555_I2C_BUS, &bus);
	if (ret) {
		/* Try to find any available I2C bus */
		for (bus_num = 0; bus_num < 4; bus_num++) {
			ret = uclass_get_device_by_seq(UCLASS_I2C, bus_num, &bus);
			if (!ret)
				break;
		}
		if (ret) {
			printf("NCA9555: No I2C bus available\n");
			return ret;
		}
	}

	/* Ensure I2C bus is probed/initialized */
	ret = device_probe(bus);
	if (ret && ret != -EPERM) {
		printf("NCA9555: Failed to probe I2C bus: %d\n", ret);
		return ret;
	}

	/* Wait a bit for I2C bus to be ready */
	mdelay(10);

	/* Try to probe NCA9555 device */
	ret = dm_i2c_probe(bus, NCA9555_I2C_ADDR, 0, &dev);
	if (ret) {
		/* Try scanning common addresses */
		for (test_addr = 0x20; test_addr <= 0x27; test_addr++) {
			ret = dm_i2c_probe(bus, test_addr, 0, &dev);
			if (!ret)
				break;
		}
		if (ret) {
			printf("NCA9555: Device not found on I2C bus\n");
			return ret;
		}
	}

	/* Set offset length to 1 byte */
	ret = i2c_set_chip_offset_len(dev, 1);
	if (ret) {
		printf("NCA9555: Failed to set offset length: %d\n", ret);
		return ret;
	}

	/* Read current configuration register (Port 0) */
	ret = dm_i2c_reg_read(dev, NCA9555_REG_CONFIG_PORT0);
	if (ret < 0) {
		printf("NCA9555: Failed to read config register: %d\n", ret);
		return ret;
	}
	config_val = (u8)ret;

	/* Configure IO0_2 as output (clear bit 2: 0=output, 1=input) */
	config_val &= ~NCA9555_IO0_2_BIT;
	ret = dm_i2c_reg_write(dev, NCA9555_REG_CONFIG_PORT0, config_val);
	if (ret) {
		printf("NCA9555: Failed to write config register: %d\n", ret);
		return ret;
	}

	/* Read current output register (Port 0) */
	ret = dm_i2c_reg_read(dev, NCA9555_REG_OUTPUT_PORT0);
	if (ret < 0) {
		printf("NCA9555: Failed to read output register: %d\n", ret);
		return ret;
	}
	output_val = (u8)ret;

	/* Pull IO0_2 low (clear bit 2) */
	output_val &= ~NCA9555_IO0_2_BIT;
	ret = dm_i2c_reg_write(dev, NCA9555_REG_OUTPUT_PORT0, output_val);
	if (ret) {
		printf("NCA9555: Failed to write output register (low): %d\n", ret);
		return ret;
	}

	/* Wait for reset pulse (10ms) */
	mdelay(10);

	/* Pull IO0_2 high (set bit 2) */
	output_val |= NCA9555_IO0_2_BIT;
	ret = dm_i2c_reg_write(dev, NCA9555_REG_OUTPUT_PORT0, output_val);
	if (ret) {
		printf("NCA9555: Failed to write output register (high): %d\n", ret);
		return ret;
	}

	return 0;
}

#ifdef CONFIG_BOARD_LATE_INIT
int board_late_init(void)
{
	if (IS_ENABLED(CONFIG_TI_I2C_BOARD_DETECT)) {
		//setup_board_eeprom_env();
		setup_serial();
	}
	       if (IS_ENABLED(CONFIG_PMIC_TPS65219)) {
               do_pmic_init();
       }

	/* Reset device via NCA9555 IO0_2 */
	nca9555_reset_io0_2();

	ti_set_fdt_env(NULL, NULL);
	return 0;
}
#endif

#if defined(CONFIG_XPL_BUILD)
void spl_board_init(void)
{
	u32 val;

	/* We have 32k crystal, so lets enable it */
	val = readl(MCU_CTRL_LFXOSC_CTRL);
	val &= ~(MCU_CTRL_LFXOSC_32K_DISABLE_VAL);
	writel(val, MCU_CTRL_LFXOSC_CTRL);
	/* Add any TRIM needed for the crystal here.. */
	/* Make sure to mux up to take the SoC 32k from the crystal */
	writel(MCU_CTRL_DEVICE_CLKOUT_LFOSC_SELECT_VAL,
	       MCU_CTRL_DEVICE_CLKOUT_32K_CTRL);

	enable_caches();
	if (IS_ENABLED(CONFIG_SPL_SPLASH_SCREEN) && IS_ENABLED(CONFIG_SPL_BMP))
		splash_display();
}

void spl_perform_fixups(struct spl_image_info *spl_image)
{
	if (IS_ENABLED(CONFIG_K3_DDRSS)) {
		if (IS_ENABLED(CONFIG_K3_INLINE_ECC))
			fixup_ddr_driver_for_ecc(spl_image);
	} else {
		fixup_memory_node(spl_image);
	}
}
#endif


#if defined(CONFIG_OF_BOARD_SETUP)
int ft_board_setup(void *blob, struct bd_info *bd)
{
	int ret = -1;

	if (IS_ENABLED(CONFIG_FDT_SIMPLEFB))
		ret = fdt_simplefb_enable_and_mem_rsv(blob);

	/* If simplefb is not enabled and video is active, then at least reserve
	 * the framebuffer region to preserve the splash screen while OS is booting
	 */
	if (IS_ENABLED(CONFIG_VIDEO) && IS_ENABLED(CONFIG_OF_LIBFDT)) {
		if (ret && video_is_active())
			return fdt_add_fb_mem_rsv(blob);
	}

	return 0;
}
#endif
