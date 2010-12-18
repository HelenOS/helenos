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
#ifndef DRV_UHCI_TD_PORT_STATUS_H
#define DRV_UHCI_TD_PORT_STATUS_H

#include <stdint.h>

struct port_register {
	uint8_t connected:1;
	uint8_t connect_change:1;
	uint8_t enabled:1;
	uint8_t enabled_change:1;
	uint8_t line:2;
	uint8_t resume:1;
	const uint8_t always_one:1; /* reserved */

	uint8_t low_speed:1;
	uint8_t reset:1;
	uint8_t :2; /* reserved */
	uint8_t suspended:1;
	uint8_t :3; /* reserved */
	/* first byte */
//	uint8_t :3; /* reserved */
//	uint8_t suspended:1;
//	uint8_t :2; /* reserved */
//	uint8_t reset:1;
//	uint8_t low_speed:1;

	/* second byte */
//	uint8_t :1; /* reserved */
//	uint8_t resume:1;
//	uint8_t line:2;
//	uint8_t enabled_change:1;
//	uint8_t enabled:1;
//	uint8_t connect_change:1;
//	uint8_t connected:1;
} __attribute__((packed));

typedef union port_status {
	struct port_register status;
	uint16_t raw_value;
} port_status_t;

void print_port_status( const port_status_t *status );
#endif
/**
 * @}
 */
