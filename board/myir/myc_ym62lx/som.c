// SPDX-License-Identifier: GPL-2.0+
/*
 * Board specific initialization for AM62Lx platforms
 *
 * Copyright (C) 2025 Texas Instruments Incorporated - https://www.ti.com/
 *
 */

#include <asm/arch/hardware.h>
#include <asm/io.h>
#include <dm/uclass.h>
#include <env.h>
#include <fdt_support.h>
#include <spl.h>
#include <linux/delay.h>
#include <asm-generic/gpio.h>

#include "../common/fdt_ops.h"

int board_init(void)
{
	int ret;
	struct gpio_desc desc_eth0;
	struct gpio_desc desc_eth1;
	ret = dm_gpio_lookup_name("gpio@600000_87", &desc_eth0);
	if (ret < 0) {
		pr_err("Failed to lookup gpio@600000_87 : %d\n", ret);
		return ret;
	}

	ret = dm_gpio_lookup_name("gpio@600000_89", &desc_eth1);
	if (ret < 0) {
		pr_err("Failed to lookup gpio@600000_89 : %d\n", ret);
		return ret;
	}

	ret = dm_gpio_request(&desc_eth0, "eth0_rst");
	if (ret < 0) {
		pr_err("Failed to request gpio@600000_87 : %d\n", ret);
		return ret;
	}

	ret = dm_gpio_request(&desc_eth1, "eth1_rst");
	if (ret < 0) {
		pr_err("Failed to request gpio@600000_89 : %d\n", ret);
		return ret;
	}

	dm_gpio_set_dir_flags(&desc_eth0, GPIOD_IS_OUT);
	dm_gpio_set_dir_flags(&desc_eth1, GPIOD_IS_OUT);

	dm_gpio_set_value(&desc_eth0, 0);
	dm_gpio_set_value(&desc_eth1, 0);
	mdelay(10);
	dm_gpio_set_value(&desc_eth0, 1);
	dm_gpio_set_value(&desc_eth1, 1);

	return 0;
}

#if IS_ENABLED(CONFIG_BOARD_LATE_INIT)
int board_late_init(void)
{
	ti_set_fdt_env(NULL, NULL);
	return 0;
}
#endif
