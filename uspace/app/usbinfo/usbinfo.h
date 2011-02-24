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
#include <usb/debug.h>
#include <ipc/devman.h>


#define NAME "usbinfo"

void dump_buffer(const char *, size_t, const uint8_t *, size_t);
const char *get_indent(size_t);
void dump_match_ids(match_id_list_t *matches);
void dump_usb_descriptor(uint8_t *, size_t);
int dump_device(devman_handle_t, usb_address_t);
void dump_descriptor_tree(uint8_t *, size_t);

static inline void internal_error(int err)
{
	fprintf(stderr, NAME ": internal error (%s).\n", str_error(err));
}

#endif
/**
 * @}
 */
