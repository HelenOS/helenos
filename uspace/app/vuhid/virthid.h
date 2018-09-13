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
/** @file
 *
 */
#ifndef VUHID_VIRTHID_H_
#define VUHID_VIRTHID_H_

#include <usb/usb.h>
#include <usbvirt/device.h>
#include <fibril_synch.h>

#define VUHID_ENDPOINT_MAX USB_ENDPOINT_MAX
#define VUHID_INTERFACE_MAX 8

typedef struct vuhid_interface vuhid_interface_t;

typedef struct {
	vuhid_interface_t *in_endpoints_mapping[VUHID_ENDPOINT_MAX];
	size_t in_endpoint_first_free;
	vuhid_interface_t *out_endpoints_mapping[VUHID_ENDPOINT_MAX];
	size_t out_endpoint_first_free;
	vuhid_interface_t *interface_mapping[VUHID_INTERFACE_MAX];

	fibril_mutex_t iface_count_mutex;
	fibril_condvar_t iface_count_cv;
	size_t iface_count;
	size_t iface_died_count;
} vuhid_data_t;

struct vuhid_interface {
	const char *name;
	const char *id;
	int usb_subclass;
	int usb_protocol;

	uint8_t *report_descriptor;
	size_t report_descriptor_size;

	size_t in_data_size;
	size_t out_data_size;

	errno_t (*on_data_in)(vuhid_interface_t *, void *, size_t, size_t *);
	errno_t (*on_data_out)(vuhid_interface_t *, const void *, size_t);
	void (*live)(vuhid_interface_t *);

	int set_protocol;

	void *interface_data;

	vuhid_data_t *vuhid_data;
};

typedef struct {
	/** Buffer with data from device to the host. */
	uint8_t *data_in;
	/** Number of items in @c data_in.
	 * The total size of @c data_in buffer shall be
	 * <code>data_in_count * vuhid_interface_t.in_data_size</code>.
	 */
	size_t data_in_count;

	/** Current position in the data buffer. */
	size_t data_in_pos;
	/** Previous position. */
	size_t data_in_last_pos;

	/** Delay between transition to "next" input buffer (in ms). */
	size_t data_in_pos_change_delay;

	/** Message to print when interface becomes alive. */
	const char *msg_born;
	/** Message to print when interface dies. */
	const char *msg_die;
} vuhid_interface_life_t;

typedef struct {
	uint8_t length;
	uint8_t type;
	uint16_t hid_spec_release;
	uint8_t country_code;
	uint8_t descriptor_count;
	uint8_t descriptor1_type;
	uint16_t descriptor1_length;
} __attribute__((packed)) hid_descriptor_t;

errno_t add_interface_by_id(vuhid_interface_t **, const char *, usbvirt_device_t *);
void wait_for_interfaces_death(usbvirt_device_t *);

void interface_life_live(vuhid_interface_t *);
errno_t interface_live_on_data_in(vuhid_interface_t *, void *, size_t, size_t *);

#endif
/**
 * @}
 */
