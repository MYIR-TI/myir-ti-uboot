// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Library to support FDT file operations which are common
 *
 * Copyright (C) 2024 Texas Instruments Incorporated - https://www.ti.com/
 */

#include <env.h>
#include <stdio.h>
#include <asm/io.h>
#include "fdt_ops.h"


void ti_set_fdt_env(const char *board_name, struct ti_fdt_map *fdt_map)
{
        char *fdt_file_name = NULL;
        char fdtfile[TI_FDT_FILE_MAX];
        u32 val;
        char default_dtb[20];

        val = readl(WKUP_MMR0_JTAG_USER_ID);
        printf("myir device\n");

        switch(val){
                case 0x3a08e4e1:
                        printf("myir device: 6231\n");
                        strcpy(default_dtb, "myir/myd-am62x-6231.dtb");
                        env_set("default_device_tree", default_dtb);
                        env_set("fdtfile", default_dtb);
                        return; 

                case 0x3a146521:
                        printf("myir device: 6252\n");
                        strcpy(default_dtb, "myir/myd-am62x-6252.dtb");
                        env_set("default_device_tree", default_dtb);
                        env_set("fdtfile", default_dtb);
                        return; 

                default:
                        printf("myir device: 6254\n");
                        strcpy(default_dtb, "myir/myd-am62x-6254.dtb");
                        env_set("default_device_tree", default_dtb);
                        env_set("fdtfile", default_dtb);
                        return; 
        }

        if (board_name) {
                while (fdt_map) {
                        /* Check for NULL terminator in the list */
                        if (!fdt_map->board_name)
                                break;
                        if (!strncmp(fdt_map->board_name, board_name, TI_BOARD_NAME_MAX)) {
                                fdt_file_name = fdt_map->fdt_file_name;
                                break;
                        }
                        fdt_map++;
                }
        }

        /* match not found OR null board_name */
        if (!fdt_file_name) {
                /*
                 * Prioritize CONFIG_DEFAULT_FDT_FILE - if that is not defined,
                 * or is empty, then use CONFIG_DEFAULT_DEVICE_TREE
                 */
#ifdef CONFIG_DEFAULT_FDT_FILE
                if (strlen(CONFIG_DEFAULT_FDT_FILE)) {
                        snprintf(fdtfile, sizeof(fdtfile), "%s", CONFIG_DEFAULT_FDT_FILE);
                } else
#endif
                {
                        snprintf(fdtfile, sizeof(fdtfile), "%s.dtb", CONFIG_DEFAULT_DEVICE_TREE);
                }
        } else {
                snprintf(fdtfile, sizeof(fdtfile), "%s", fdt_file_name);
        }

        env_set("fdtfile", fdtfile);

        /*
         * XXX: DEPRECATION WARNING: 2 u-boot versions (2024.10).
         *
         * Maintain compatibility with downstream scripts that may be using
         * name_fdt
         */
        if (board_name)
                env_set("name_fdt", fdtfile);
        /* Also set the findfdt legacy script to warn users to stop using this */
        env_set("findfdt",
                "echo WARN: fdtfile already set. Stop using findfdt in script");
}
