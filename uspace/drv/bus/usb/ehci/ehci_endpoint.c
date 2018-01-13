/*
 * Copyright (c) 2014 Jan Vesely
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
/** @addtogroup drvusbehci
 * @{
 */
/** @file
 * @brief EHCI driver
 */

#include <assert.h>
#include <stdlib.h>
#include <usb/debug.h>
#include <usb/host/utils/malloc32.h>

#include "ehci_endpoint.h"
#include "hc.h"

/** Callback to set toggle on ED.
 *
 * @param[in] hcd_ep hcd endpoint structure
 * @param[in] toggle new value of toggle bit
 */
static void ehci_ep_toggle_set(void *ehci_ep, int toggle)
{
	ehci_endpoint_t *instance = ehci_ep;
	assert(instance);
	assert(instance->qh);
	if (qh_toggle_from_td(instance->qh))
		usb_log_warning("EP(%p): Setting toggle bit for transfer "
		    "directed EP", instance);
	qh_toggle_set(instance->qh, toggle);
}

/** Callback to get value of toggle bit.
 *
 * @param[in] hcd_ep hcd endpoint structure
 * @return Current value of toggle bit.
 */
static int ehci_ep_toggle_get(void *ehci_ep)
{
	ehci_endpoint_t *instance = ehci_ep;
	assert(instance);
	assert(instance->qh);
	if (qh_toggle_from_td(instance->qh))
		usb_log_warning("EP(%p): Reading useless toggle bit", instance);
	return qh_toggle_get(instance->qh);
}

/** Creates new hcd endpoint representation.
 *
 * @param[in] ep USBD endpoint structure
 * @return Error code.
 */
errno_t ehci_endpoint_init(hcd_t *hcd, endpoint_t *ep)
{
	assert(ep);
	hc_t *hc = hcd_get_driver_data(hcd);

	ehci_endpoint_t *ehci_ep = malloc(sizeof(ehci_endpoint_t));
	if (ehci_ep == NULL)
		return ENOMEM;

	ehci_ep->qh = malloc32(sizeof(qh_t));
	if (ehci_ep->qh == NULL) {
		free(ehci_ep);
		return ENOMEM;
	}

	usb_log_debug2("EP(%p): Creating for %p", ehci_ep, ep);
	link_initialize(&ehci_ep->link);
	qh_init(ehci_ep->qh, ep);
	endpoint_set_hc_data(
	    ep, ehci_ep, ehci_ep_toggle_get, ehci_ep_toggle_set);
	hc_enqueue_endpoint(hc, ep);
	return EOK;
}

/** Disposes hcd endpoint structure
 *
 * @param[in] hcd driver using this instance.
 * @param[in] ep endpoint structure.
 */
void ehci_endpoint_fini(hcd_t *hcd, endpoint_t *ep)
{
	assert(hcd);
	assert(ep);
	hc_t *hc = hcd_get_driver_data(hcd);

	ehci_endpoint_t *instance = ehci_endpoint_get(ep);
	hc_dequeue_endpoint(hc, ep);
	endpoint_clear_hc_data(ep);
	usb_log_debug2("EP(%p): Destroying for %p", instance, ep);
	if (instance) {
		free32(instance->qh);
		free(instance);
	}
}
/**
 * @}
 */

