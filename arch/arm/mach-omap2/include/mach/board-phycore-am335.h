/*
 * Code for supporting phyBOARD phyCORE-AM335
 *
 * Copyright (C) {2013} PHYTEC Embedded pvt. Ltd. - http://www.phytec.in
 *
 * Based on AM335X EVM.
 *
 * Copyright (C) {2011} Texas Instruments Incorporated - http://www.ti.com/
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

#ifndef _BOARD_PHYCORE_AM335_H
#define _BOARD_PHYCORE_AM335_H

#define CB_STR_LEN 20

static unsigned int valid_brd_name;

struct phycore_am335_carrier {
	char *board_name;
	void (*devinit) (void);
};

struct carrier_board_list {
	char *name;
};

static struct carrier_board_list list_boards[] = {
	{ "pba-c-01" },
};

#endif
