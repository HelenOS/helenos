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

#include <errno.h>

#include "endpoint.h"

int endpoint_init(hcd_t *hcd, endpoint_t *ep)
{
	assert(ep);
	xhci_endpoint_t *xhci_ep = malloc(sizeof(xhci_endpoint_t));
	if (xhci_ep == NULL)
		return ENOMEM;

	/* FIXME: Set xhci_ep->slot_id */

	fibril_mutex_lock(&ep->guard);
	ep->hc_data.data = xhci_ep;
	/* FIXME: The two handlers below should be implemented. */
	ep->hc_data.toggle_get = NULL;
	ep->hc_data.toggle_set = NULL;
	fibril_mutex_unlock(&ep->guard);

	usb_log_debug("Endpoint %d:%d initialized.", ep->address, ep->endpoint);

	return EOK;
}

void endpoint_fini(hcd_t *hcd, endpoint_t *ep)
{
	assert(hcd);
	assert(ep);
	xhci_endpoint_t *xhci_ep = endpoint_get(ep);
	/* FIXME: Tear down TR's? */
	if (xhci_ep) {
		free(xhci_ep);
	}

	fibril_mutex_lock(&ep->guard);
	ep->hc_data.data = NULL;
	ep->hc_data.toggle_get = NULL;
	ep->hc_data.toggle_set = NULL;
	fibril_mutex_unlock(&ep->guard);

	usb_log_debug("Endpoint %d:%d destroyed.", ep->address, ep->endpoint);
}

/**
 * @}
 */
