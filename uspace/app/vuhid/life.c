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
 *
 */
#include <errno.h>
#include <usb/debug.h>
#include "virthid.h"

void interface_life_live(vuhid_interface_t *iface)
{
	vuhid_interface_life_t *data = iface->interface_data;
	data->data_in_pos = 0;
	data->data_in_last_pos = (size_t) -1;
	fibril_usleep(1000 * 1000 * 5);
	usb_log_debug("%s", data->msg_born);
	while (data->data_in_pos < data->data_in_count) {
		fibril_usleep(1000 * data->data_in_pos_change_delay);
		// FIXME: proper locking
		data->data_in_pos++;
	}
	usb_log_debug("%s", data->msg_die);
}

errno_t interface_live_on_data_in(vuhid_interface_t *iface,
    void *buffer, size_t buffer_size, size_t *act_buffer_size)
{
	vuhid_interface_life_t *life = iface->interface_data;
	size_t pos = life->data_in_pos;
	if (pos >= life->data_in_count) {
		return EBADCHECKSUM;
	}

	if (pos == life->data_in_last_pos) {
		return ENAK;
	}

	if (buffer_size > iface->in_data_size) {
		buffer_size = iface->in_data_size;
	}

	if (act_buffer_size != NULL) {
		*act_buffer_size = buffer_size;
	}

	memcpy(buffer, life->data_in + pos * iface->in_data_size, buffer_size);
	life->data_in_last_pos = pos;

	return EOK;
}

/** @}
 */
