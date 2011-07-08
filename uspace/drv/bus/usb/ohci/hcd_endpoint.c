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
#include "utils/malloc32.h"
#include "hcd_endpoint.h"

/** Callback to set toggle on ED.
 *
 * @param[in] hcd_ep hcd endpoint structure
 * @param[in] toggle new value of toggle bit
 */
static void hcd_ep_toggle_set(void *hcd_ep, int toggle)
{
	hcd_endpoint_t *instance = hcd_ep;
	assert(instance);
	assert(instance->ed);
	ed_toggle_set(instance->ed, toggle);
}
/*----------------------------------------------------------------------------*/
/** Callback to get value of toggle bit.
 *
 * @param[in] hcd_ep hcd endpoint structure
 * @return Current value of toggle bit.
 */
static int hcd_ep_toggle_get(void *hcd_ep)
{
	hcd_endpoint_t *instance = hcd_ep;
	assert(instance);
	assert(instance->ed);
	return ed_toggle_get(instance->ed);
}
/*----------------------------------------------------------------------------*/
/** Creates new hcd endpoint representation.
 *
 * @param[in] ep USBD endpoint structure
 * @return pointer to a new hcd endpoint structure, NULL on failure.
 */
hcd_endpoint_t * hcd_endpoint_assign(endpoint_t *ep)
{
	assert(ep);
	hcd_endpoint_t *hcd_ep = malloc(sizeof(hcd_endpoint_t));
	if (hcd_ep == NULL)
		return NULL;

	hcd_ep->ed = malloc32(sizeof(ed_t));
	if (hcd_ep->ed == NULL) {
		free(hcd_ep);
		return NULL;
	}

	hcd_ep->td = malloc32(sizeof(td_t));
	if (hcd_ep->td == NULL) {
		free32(hcd_ep->ed);
		free(hcd_ep);
		return NULL;
	}

	ed_init(hcd_ep->ed, ep);
	ed_set_td(hcd_ep->ed, hcd_ep->td);
	endpoint_set_hc_data(ep, hcd_ep, hcd_ep_toggle_get, hcd_ep_toggle_set);

	return hcd_ep;
}
/*----------------------------------------------------------------------------*/
/** Disposes assigned hcd endpoint structure
 *
 * @param[in] ep USBD endpoint structure
 */
void hcd_endpoint_clear(endpoint_t *ep)
{
	assert(ep);
	hcd_endpoint_t *hcd_ep = ep->hc_data.data;
	assert(hcd_ep);
	free32(hcd_ep->ed);
	free32(hcd_ep->td);
	free(hcd_ep);
}
/**
 * @}
 */
