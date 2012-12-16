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

/** @addtogroup libusbhost
 * @{
 */
/** @file
 *
 */

#include <errno.h>
#include <str_error.h>
#include <usb/debug.h>

#include <usb/host/hcd.h>

/** Announce root hub to the DDF
 *
 * @param[in] instance OHCI driver instance
 * @param[in] hub_fun DDF function representing OHCI root hub
 * @return Error code
 */
int hcd_register_hub(hcd_t *instance, usb_address_t *address, ddf_fun_t *hub_fun)
{
	assert(instance);
	assert(address);
	assert(hub_fun);

	int ret = usb_device_manager_request_address(
	    &instance->dev_manager, address, false,
	    USB_SPEED_FULL);
	if (ret != EOK) {
		usb_log_error("Failed to get root hub address: %s\n",
		    str_error(ret));
		return ret;
	}

#define CHECK_RET_UNREG_RETURN(ret, message...) \
if (ret != EOK) { \
	usb_log_error(message); \
	usb_endpoint_manager_remove_ep( \
	    &instance->ep_manager, *address, 0, \
	    USB_DIRECTION_BOTH, NULL, NULL); \
	usb_device_manager_release_address( \
	    &instance->dev_manager, *address); \
	return ret; \
} else (void)0

	ret = usb_endpoint_manager_add_ep(
	    &instance->ep_manager, *address, 0,
	    USB_DIRECTION_BOTH, USB_TRANSFER_CONTROL, USB_SPEED_FULL, 64,
	    0, NULL, NULL);
	CHECK_RET_UNREG_RETURN(ret,
	    "Failed to register root hub control endpoint: %s.\n",
	    str_error(ret));

	ret = ddf_fun_add_match_id(hub_fun, "usb&class=hub", 100);
	CHECK_RET_UNREG_RETURN(ret,
	    "Failed to add root hub match-id: %s.\n", str_error(ret));

	ret = ddf_fun_bind(hub_fun);
	CHECK_RET_UNREG_RETURN(ret,
	    "Failed to bind root hub function: %s.\n", str_error(ret));

	ret = usb_device_manager_bind_address(&instance->dev_manager,
	    *address, ddf_fun_get_handle(hub_fun));
	if (ret != EOK)
		usb_log_warning("Failed to bind root hub address: %s.\n",
		    str_error(ret));

	return EOK;
#undef CHECK_RET_UNREG_RETURN
}

/**
 * @}
 */
