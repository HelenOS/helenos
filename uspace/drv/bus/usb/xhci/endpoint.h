/*
 * Copyright (c) 2017 Petr Manek
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

/** @addtogroup drvusbxhci
 * @{
 */
/** @file
 * @brief The host controller endpoint management.
 */

#ifndef XHCI_ENDPOINT_H
#define XHCI_ENDPOINT_H

#include <assert.h>

#include <usb/debug.h>
#include <usb/host/endpoint.h>
#include <usb/host/hcd.h>

enum {
	EP_TYPE_INVALID = 0,
	EP_TYPE_ISOCH_OUT = 1,
	EP_TYPE_BULK_OUT = 2,
	EP_TYPE_INTERRUPT_OUT = 3,
	EP_TYPE_CONTROL = 4,
	EP_TYPE_ISOCH_IN = 5,
	EP_TYPE_BULK_IN = 6,
	EP_TYPE_INTERRUPT_IN = 7
};

/** Connector structure linking endpoint context to the endpoint. */
typedef struct xhci_endpoint {
	uint32_t slot_id;
} xhci_endpoint_t;

int endpoint_init(hcd_t *hcd, endpoint_t *ep);
void endpoint_fini(hcd_t *hcd, endpoint_t *ep);

static inline xhci_endpoint_t * endpoint_get(const endpoint_t *ep)
{
	assert(ep);
	return ep->hc_data.data;
}

#endif

/**
 * @}
 */
