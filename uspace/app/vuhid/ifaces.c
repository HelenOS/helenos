/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup usbvirthid
 * @{
 */
/**
 * @file
 * List of known interfaces.
 */
#include <stdlib.h>
#include "ifaces.h"

extern vuhid_interface_t vuhid_interface_bootkbd;
extern vuhid_interface_t vuhid_interface_logitech_wireless_1;

vuhid_interface_t *available_hid_interfaces[] = {
	&vuhid_interface_bootkbd,
	&vuhid_interface_logitech_wireless_1,
	NULL
};

/** @}
 */
