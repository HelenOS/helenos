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

/** @addtogroup drvusbohci
 * @{
 */
/** @file
 * @brief OHCI driver
 */

#include <assert.h>
#include <stdlib.h>
#include <usb/host/utils/malloc32.h>

#include "ohci_endpoint.h"
#include "hc.h"

/** Callback to set toggle on ED.
 *
 * @param[in] hcd_ep hcd endpoint structure
 * @param[in] toggle new value of toggle bit
 */
static void ohci_ep_toggle_set(void *ohci_ep, int toggle)
{
	ohci_endpoint_t *instance = ohci_ep;
	assert(instance);
	assert(instance->ed);
	ed_toggle_set(instance->ed, toggle);
}

/** Callback to get value of toggle bit.
 *
 * @param[in] hcd_ep hcd endpoint structure
 * @return Current value of toggle bit.
 */
static int ohci_ep_toggle_get(void *ohci_ep)
{
	ohci_endpoint_t *instance = ohci_ep;
	assert(instance);
	assert(instance->ed);
	return ed_toggle_get(instance->ed);
}

/** Creates new hcd endpoint representation.
 *
 * @param[in] ep USBD endpoint structure
 * @return Error code.
 */
errno_t ohci_endpoint_init(hcd_t *hcd, endpoint_t *ep)
{
	assert(ep);
	ohci_endpoint_t *ohci_ep = malloc(sizeof(ohci_endpoint_t));
	if (ohci_ep == NULL)
		return ENOMEM;

	ohci_ep->ed = malloc32(sizeof(ed_t));
	if (ohci_ep->ed == NULL) {
		free(ohci_ep);
		return ENOMEM;
	}

	ohci_ep->td = malloc32(sizeof(td_t));
	if (ohci_ep->td == NULL) {
		free32(ohci_ep->ed);
		free(ohci_ep);
		return ENOMEM;
	}

	link_initialize(&ohci_ep->link);
	ed_init(ohci_ep->ed, ep, ohci_ep->td);
	endpoint_set_hc_data(
	    ep, ohci_ep, ohci_ep_toggle_get, ohci_ep_toggle_set);
	hc_enqueue_endpoint(hcd_get_driver_data(hcd), ep);
	return EOK;
}

/** Disposes hcd endpoint structure
 *
 * @param[in] hcd driver using this instance.
 * @param[in] ep endpoint structure.
 */
void ohci_endpoint_fini(hcd_t *hcd, endpoint_t *ep)
{
	assert(hcd);
	assert(ep);
	ohci_endpoint_t *instance = ohci_endpoint_get(ep);
	hc_dequeue_endpoint(hcd_get_driver_data(hcd), ep);
	endpoint_clear_hc_data(ep);
	if (instance) {
		free32(instance->ed);
		free32(instance->td);
		free(instance);
	}
}

/**
 * @}
 */
