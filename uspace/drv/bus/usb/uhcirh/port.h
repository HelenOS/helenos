/*
 * Copyright (c) 2011 Jan Vesely
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
/** @addtogroup drvusbuhcirh
 * @{
 */
/** @file
 * @brief UHCI root hub port routines
 */
#ifndef DRV_UHCI_PORT_H
#define DRV_UHCI_PORT_H

#include <stdint.h>
#include <fibril.h>
#include <ddf/driver.h>
#include <usb/hc.h> /* usb_hc_connection_t */
#include <usb/dev/hub.h>

typedef uint16_t port_status_t;
#define STATUS_CONNECTED         (1 << 0)
#define STATUS_CONNECTED_CHANGED (1 << 1)
#define STATUS_ENABLED           (1 << 2)
#define STATUS_ENABLED_CHANGED   (1 << 3)
#define STATUS_LINE_D_PLUS       (1 << 4)
#define STATUS_LINE_D_MINUS      (1 << 5)
#define STATUS_RESUME            (1 << 6)
#define STATUS_ALWAYS_ONE        (1 << 7)

#define STATUS_LOW_SPEED (1 <<  8)
#define STATUS_IN_RESET  (1 <<  9)
#define STATUS_SUSPEND   (1 << 12)

/** UHCI port structure */
typedef struct uhci_port {
	const char *id_string;
	port_status_t *address;
	unsigned number;
	unsigned wait_period_usec;
	usb_hc_connection_t hc_connection;
	ddf_dev_t *rh;
	usb_hub_attached_device_t attached_device;
	fid_t checker;
} uhci_port_t;

int uhci_port_init(
    uhci_port_t *port, port_status_t *address, unsigned number,
    unsigned usec, ddf_dev_t *rh);

void uhci_port_fini(uhci_port_t *port);

#endif
/**
 * @}
 */
