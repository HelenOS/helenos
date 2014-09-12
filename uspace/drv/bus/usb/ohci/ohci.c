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

#include <errno.h>
#include <str_error.h>
#include <ddf/interrupt.h>
#include <usb_iface.h>
#include <usb/usb.h>
#include <usb/ddfiface.h>
#include <usb/debug.h>

#include "ohci.h"
#include "res.h"
#include "hc.h"

typedef struct ohci {
	ddf_fun_t *hc_fun;
	ddf_fun_t *rh_fun;

	hc_t hc;
} ohci_t;

static inline ohci_t *dev_to_ohci(ddf_dev_t *dev)
{
	return ddf_dev_data_get(dev);
}
/** IRQ handling callback, identifies device
 *
 * @param[in] iid (Unused).
 * @param[in] call Pointer to the call that represents interrupt.
 * @param[in] dev DDF instance of the device to use.
 *
 */
static void irq_handler(ipc_callid_t iid, ipc_call_t *call, ddf_dev_t *dev)
{
	assert(dev);
	
	ohci_t *ohci = dev_to_ohci(dev);
	if (!ohci) {
		usb_log_warning("Interrupt on device that is not ready.\n");
		return;
	}
	
	const uint16_t status = IPC_GET_ARG1(*call);
	hc_interrupt(&ohci->hc, status);
}

/** Get USB address assigned to root hub.
 *
 * @param[in] fun Root hub function.
 * @param[out] address Store the address here.
 * @return Error code.
 */
static int rh_get_my_address(ddf_fun_t *fun, usb_address_t *address)
{
	assert(fun);

	if (address != NULL) {
		*address = dev_to_ohci(ddf_fun_get_dev(fun))->hc.rh.address;
	}

	return EOK;
}

/** Gets handle of the respective hc (this device, hc function).
 *
 * @param[in] root_hub_fun Root hub function seeking hc handle.
 * @param[out] handle Place to write the handle.
 * @return Error code.
 */
static int rh_get_hc_handle(
    ddf_fun_t *fun, devman_handle_t *handle)
{
	assert(fun);
	ddf_fun_t *hc_fun = dev_to_ohci(ddf_fun_get_dev(fun))->hc_fun;
	assert(hc_fun);

	if (handle != NULL)
		*handle = ddf_fun_get_handle(hc_fun);
	return EOK;
}

/** Root hub USB interface */
static usb_iface_t usb_iface = {
	.get_hc_handle = rh_get_hc_handle,
	.get_my_address = rh_get_my_address,
};

/** Standard USB HC options (HC interface) */
static ddf_dev_ops_t hc_ops = {
	.interfaces[USBHC_DEV_IFACE] = &hcd_iface,
};

/** Standard USB RH options (RH interface) */
static ddf_dev_ops_t rh_ops = {
	.interfaces[USB_DEV_IFACE] = &usb_iface,
};

/** Initialize hc and rh ddf structures and their respective drivers.
 *
 * @param[in] device DDF instance of the device to use.
 * @param[in] instance OHCI structure to use.
 *
 * This function does all the preparatory work for hc and rh drivers:
 *  - gets device hw resources
 *  - disables OHCI legacy support
 *  - asks for interrupt
 *  - registers interrupt handler
 */
int device_setup_ohci(ddf_dev_t *device)
{
	bool ih_registered = false;
	bool hc_inited = false;
	int rc;

	if (device == NULL)
		return EBADMEM;

	ohci_t *instance = ddf_dev_data_alloc(device,sizeof(ohci_t));
	if (instance == NULL) {
		usb_log_error("Failed to allocate OHCI driver.\n");
		return ENOMEM;
	}

	instance->hc_fun = ddf_fun_create(device, fun_exposed, "ohci_hc");
	if (instance->hc_fun == NULL) {
		usb_log_error("Failed to create OHCI HC function: %s.\n",
		    str_error(ENOMEM));
		rc = ENOMEM;
		goto error;
	}

	ddf_fun_set_ops(instance->hc_fun, &hc_ops);

	instance->rh_fun = ddf_fun_create(device, fun_inner, "ohci_rh");
	if (instance->rh_fun == NULL) {
		usb_log_error("Failed to create OHCI RH function: %s.\n",
		    str_error(ENOMEM));
		rc = ENOMEM;
		goto error;
	}

	ddf_fun_set_ops(instance->rh_fun, &rh_ops);

	addr_range_t regs;
	int irq = 0;

	rc = get_my_registers(device, &regs, &irq);
	if (rc != EOK) {
		usb_log_error("Failed to get register memory addresses "
		    "for %" PRIun ": %s.\n", ddf_dev_get_handle(device),
		    str_error(rc));
		goto error;
	}

	usb_log_debug("Memory mapped regs at %p (size %zu), IRQ %d.\n",
	    RNGABSPTR(regs), RNGSZ(regs), irq);

	rc = hc_register_irq_handler(device, &regs, irq, irq_handler);
	if (rc != EOK) {
		usb_log_error("Failed to register interrupt handler: %s.\n",
		    str_error(rc));
		goto error;
	}

	ih_registered = true;

	/* Try to enable interrupts */
	bool interrupts = false;
	rc = enable_interrupts(device);
	if (rc != EOK) {
		usb_log_warning("Failed to enable interrupts: %s."
		    " Falling back to polling\n", str_error(rc));
		/* We don't need that handler */
		unregister_interrupt_handler(device, irq);
		ih_registered = false;
	} else {
		usb_log_debug("Hw interrupts enabled.\n");
		interrupts = true;
	}

	rc = hc_init(&instance->hc, instance->hc_fun, &regs, interrupts);
	if (rc != EOK) {
		usb_log_error("Failed to init ohci_hcd: %s.\n", str_error(rc));
		goto error;
	}

	hc_inited = true;

	rc = ddf_fun_bind(instance->hc_fun);
	if (rc != EOK) {
		usb_log_error("Failed to bind OHCI device function: %s.\n",
		    str_error(rc));
		goto error;
	}

	rc = ddf_fun_add_to_category(instance->hc_fun, USB_HC_CATEGORY);
	if (rc != EOK) {
		usb_log_error("Failed to add OHCI to HC category: %s.\n",
		    str_error(rc));
		goto error;
	}

	rc = hc_register_hub(&instance->hc, instance->rh_fun);
	if (rc != EOK) {
		usb_log_error("Failed to register OHCI root hub: %s.\n",
		    str_error(rc));
		goto error;
	}

	return EOK;

error:
	if (hc_inited)
		hc_fini(&instance->hc);
	if (ih_registered)
		unregister_interrupt_handler(device, irq);
	if (instance->hc_fun != NULL)
		ddf_fun_destroy(instance->hc_fun);
	if (instance->rh_fun != NULL)
		ddf_fun_destroy(instance->rh_fun);
	return rc;
}
/**
 * @}
 */
