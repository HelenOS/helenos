/*
 * Copyright (c) 2010 Vojtech Horky
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

/** @addtogroup usbinfo
 * @{
 */
/** @file
 * Common header for usbinfo application.
 */
#ifndef USBINFO_USBINFO_H_
#define USBINFO_USBINFO_H_

#include <usb/usb.h>
#include <usb/descriptor.h>
#include <usb/dev/pipes.h>
#include <usb/debug.h>
#include <usb/dev/dp.h>
#include <ipc/devman.h>

typedef struct {
	usb_hc_connection_t hc_conn;
	usb_device_connection_t wire;
	usb_pipe_t ctrl_pipe;
	usb_standard_device_descriptor_t device_descriptor;
	uint8_t *full_configuration_descriptor;
	size_t full_configuration_descriptor_size;
} usbinfo_device_t;

typedef struct {
	int opt;
	void (*action)(usbinfo_device_t *dev);
	bool active;
} usbinfo_action_t;


#define NAME "usbinfo"

void dump_buffer(const char *, size_t, const uint8_t *, size_t);
const char *get_indent(size_t);
void dump_match_ids(match_id_list_t *, const char *);
void dump_usb_descriptor(uint8_t *, size_t);
void dump_descriptor_tree(uint8_t *, size_t);

static inline void internal_error(int err)
{
	fprintf(stderr, NAME ": internal error (%s).\n", str_error(err));
}

usbinfo_device_t *prepare_device(const char *, devman_handle_t, usb_address_t);
void destroy_device(usbinfo_device_t *);

typedef void (*dump_descriptor_in_tree_t)(const uint8_t *, size_t, void *);
void browse_descriptor_tree(uint8_t *, size_t, usb_dp_descriptor_nesting_t *,
    dump_descriptor_in_tree_t, size_t, void *);

void list(void);

void dump_short_device_identification(usbinfo_device_t *);
void dump_device_match_ids(usbinfo_device_t *);
void dump_descriptor_tree_brief(usbinfo_device_t *);
void dump_descriptor_tree_full(usbinfo_device_t *);
void dump_strings(usbinfo_device_t *);
void dump_status(usbinfo_device_t *);
void dump_hidreport_raw(usbinfo_device_t *);
void dump_hidreport_usages(usbinfo_device_t *);


#endif
/**
 * @}
 */
