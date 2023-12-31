// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright (C) 2015 Thomas Chou <thomas@wytron.com.tw>
 */

#define LOG_CATEGORY UCLASS_MTD

#include <common.h>
#include <dm.h>
#include <dm/device-internal.h>
#include <errno.h>
#include <mtd.h>

/**
 * mtd_remove - Remove the device @dev
 *
 * @dev: U-Boot device to probe
 *
 * @return 0 on success, an error otherwise.
 */
int mtd_remove(struct mtd_info *mtd)
{
	return device_remove(mtd->dev, DM_REMOVE_NORMAL);
}

/*
 * Implement a MTD uclass which should include most flash drivers.
 * The uclass private is pointed to mtd_info.
 */

UCLASS_DRIVER(mtd) = {
	.id		= UCLASS_MTD,
	.name		= "mtd",
	.per_device_auto	= sizeof(struct mtd_info),
};
