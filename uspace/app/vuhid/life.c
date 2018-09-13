/*
 * Copyright (c) 2011 Vojtech Horky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
