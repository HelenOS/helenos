/*
 * Copyright (c) 2010 Jan Vesely
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
/** @addtogroup usb
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#ifndef DRV_UHCI_TRANSFER_DESCRIPTOR_H
#define DRV_UHCI_TRANSFER_DESCRIPTOR_H

#include <mem.h>
#include <usb/usb.h>
#include "callback.h"

/** Status field in UHCI Transfer Descriptor (TD) */
typedef struct status {
	uint8_t active:1;
	uint8_t stalled:1;
	uint8_t data_buffer_error:1;
	uint8_t babble:1;
	uint8_t nak:1;
	uint8_t crc:1;
	uint8_t bitstuff:1;
	uint8_t :1; /* reserved */
} status_t;

/** UHCI Transfer Descriptor */
typedef struct transfer_descriptor {
	uint32_t fpl:28;
	char :1; /* reserved */
	uint8_t depth:1;
	uint8_t qh:1;
	uint8_t terminate:1;

	char :2; /* reserved */
	uint8_t spd:1;
	uint8_t error_count:2;
	uint8_t low_speed:1;
	uint8_t isochronous:1;
	uint8_t ioc:1;
	status_t status;
	char :5; /* reserved */
	uint16_t act_len:10;

	uint16_t maxlen:11;
	char :1; /* reserved */
	uint8_t toggle:1;
	uint8_t endpoint:4;
	uint8_t address:7;
	uint8_t pid;

	uint32_t buffer_ptr;

	/* there is 16 byte of data available here
	 * those are used to store callback pointer
	 * and next pointer. Thus there is some free space
	 * on 32bits systems.
	 */
	struct transfer_descriptor *next;
	callback_t *callback;
} __attribute__((packed)) transfer_descriptor_t;

static inline int transfer_descriptor_init(transfer_descriptor_t *instance,
  int error_count, size_t size, bool isochronous, usb_target_t target,
	int pid)
{
	assert(instance);
	bzero(instance, sizeof(transfer_descriptor_t));

	instance->depth = 1;
	instance->terminate = 1;

	assert(error_count < 4);
	instance->error_count = error_count;
	instance->status.active = 1;

	assert(size < 1024);
	instance->maxlen = size;

	instance->address = target.address;
	instance->endpoint = target.endpoint;

	instance->pid = pid;

	return EOK;
}

#endif
/**
 * @}
 */
