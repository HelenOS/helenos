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

#include <libarch/ddi.h> /* pio_read and pio_write */

#include <stdint.h>

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

static inline port_status_t port_status_read(port_status_t * address)
	{ return pio_read_16(address); }

static inline void port_status_write(
  port_status_t *address, port_status_t value)
	{ pio_write_16(address, value); }

void print_port_status(const port_status_t status);
#endif
/**
 * @}
 */
