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

/** @addtogroup drvusbuhci
 * @{
 */
/** @file
 * @brief UHCI driver
 */

#include <errno.h>
#include <stdbool.h>
#include <str_error.h>
#include <ddf/interrupt.h>
#include <usb_iface.h>
#include <usb/ddfiface.h>
#include <usb/debug.h>

#include "uhci.h"

#include "res.h"
#include "hc.h"
#include "root_hub.h"

/** Structure representing both functions of UHCI hc, USB host controller
 * and USB root hub */
typedef struct uhci {
	/** Pointer to DDF representation of UHCI host controller */
	ddf_fun_t *hc_fun;
	/** Pointer to DDF representation of UHCI root hub */
	ddf_fun_t *rh_fun;

	/** Internal driver's representation of UHCI host controller */
	hc_t hc;
	/** Internal driver's representation of UHCI root hub */
	rh_t *rh;
} uhci_t;

static inline uhci_t *dev_to_uhci(ddf_dev_t *dev)
{
	return ddf_dev_data_get(dev);
}

/** IRQ handling callback, forward status from call to diver structure.
 *
 * @param[in] iid (Unused).
 * @param[in] call Pointer to the call from kernel.
 * @param[in] dev DDF instance of the device to use.
 *
 */
static void irq_handler(ipc_callid_t iid, ipc_call_t *call, ddf_dev_t *dev)
{
	assert(dev);
	
	uhci_t *uhci = dev_to_uhci(dev);
	if (!uhci) {
		usb_log_error("Interrupt on not yet initialized device.\n");
		return;
	}
	
	const uint16_t status = IPC_GET_ARG1(*call);
	hc_interrupt(&uhci->hc, status);
}

/** Operations supported by the HC driver */
static ddf_dev_ops_t hc_ops = {
	.interfaces[USBHC_DEV_IFACE] = &hcd_iface, /* see iface.h/c */
};

/** Gets handle of the respective hc.
 *
 * @param[in] fun DDF function of uhci device.
 * @param[out] handle Host cotnroller handle.
 * @return Error code.
 */
static int usb_iface_get_hc_handle(ddf_fun_t *fun, devman_handle_t *handle)
{
	ddf_fun_t *hc_fun = dev_to_uhci(ddf_fun_get_dev(fun))->hc_fun;
	assert(hc_fun);

	if (handle != NULL)
		*handle = ddf_fun_get_handle(hc_fun);
	return EOK;
}

/** USB interface implementation used by RH */
static usb_iface_t usb_iface = {
	.get_hc_handle = usb_iface_get_hc_handle,
};

/** Get root hub hw resources (I/O registers).
 *
 * @param[in] fun Root hub function.
 * @return Pointer to the resource list used by the root hub.
 */
static hw_resource_list_t *get_resource_list(ddf_fun_t *fun)
{
	rh_t *rh = ddf_fun_data_get(fun);
	assert(rh);
	return &rh->resource_list;
}

/** Interface to provide the root hub driver with hw info */
static hw_res_ops_t hw_res_iface = {
	.get_resource_list = get_resource_list,
	.enable_interrupt = NULL,
};

static pio_window_t *get_pio_window(ddf_fun_t *fun)
{
	rh_t *rh = ddf_fun_data_get(fun);
	
	if (rh == NULL)
		return NULL;
	return &rh->pio_window;
}

static pio_window_ops_t pio_window_iface = {
	.get_pio_window = get_pio_window
};

/** RH function support for uhci_rhd */
static ddf_dev_ops_t rh_ops = {
	.interfaces[USB_DEV_IFACE] = &usb_iface,
	.interfaces[HW_RES_DEV_IFACE] = &hw_res_iface,
	.interfaces[PIO_WINDOW_DEV_IFACE] = &pio_window_iface
};

/** Initialize hc and rh DDF structures and their respective drivers.
 *
 * @param[in] device DDF instance of the device to use.
 *
 * This function does all the preparatory work for hc and rh drivers:
 *  - gets device's hw resources
 *  - disables UHCI legacy support (PCI config space)
 *  - attempts to enable interrupts
 *  - registers interrupt handler
 */
int device_setup_uhci(ddf_dev_t *device)
{
	bool ih_registered = false;
	bool hc_inited = false;
	bool fun_bound = false;
	int rc;

	if (!device)
		return EBADMEM;

	uhci_t *instance = ddf_dev_data_alloc(device, sizeof(uhci_t));
	if (instance == NULL) {
		usb_log_error("Failed to allocate OHCI driver.\n");
		return ENOMEM;
	}

	instance->hc_fun = ddf_fun_create(device, fun_exposed, "uhci_hc");
	if (instance->hc_fun == NULL) {
		usb_log_error("Failed to create UHCI HC function.\n");
		rc = ENOMEM;
		goto error;
	}

	ddf_fun_set_ops(instance->hc_fun, &hc_ops);

	instance->rh_fun = ddf_fun_create(device, fun_inner, "uhci_rh");
	if (instance->rh_fun == NULL) {
		usb_log_error("Failed to create UHCI RH function.\n");
		rc = ENOMEM;
		goto error;
	}

	ddf_fun_set_ops(instance->rh_fun, &rh_ops);
	instance->rh = ddf_fun_data_alloc(instance->rh_fun, sizeof(rh_t));

	addr_range_t regs;
	int irq = 0;

	rc = get_my_registers(device, &regs, &irq);
	if (rc != EOK) {
		usb_log_error("Failed to get I/O addresses for %" PRIun ": %s.\n",
		    ddf_dev_get_handle(device), str_error(rc));
		goto error;
	}
	usb_log_debug("I/O regs at %p (size %zu), IRQ %d.\n",
	    RNGABSPTR(regs), RNGSZ(regs), irq);

	rc = disable_legacy(device);
	if (rc != EOK) {
		usb_log_error("Failed to disable legacy USB: %s.\n",
		    str_error(rc));
		goto error;
	}

	rc = hc_register_irq_handler(device, &regs, irq, irq_handler);
	if (rc != EOK) {
		usb_log_error("Failed to register interrupt handler: %s.\n",
		    str_error(rc));
		goto error;
	}

	ih_registered = true;

	bool interrupts = false;
	rc = enable_interrupts(device);
	if (rc != EOK) {
		usb_log_warning("Failed to enable interrupts: %s."
		    " Falling back to polling.\n", str_error(rc));
	} else {
		usb_log_debug("Hw interrupts enabled.\n");
		interrupts = true;
	}

	rc = hc_init(&instance->hc, instance->hc_fun, &regs, interrupts);
	if (rc != EOK) {
		usb_log_error("Failed to init uhci_hcd: %s.\n", str_error(rc));
		goto error;
	}

	hc_inited = true;

	rc = ddf_fun_bind(instance->hc_fun);
	if (rc != EOK) {
		usb_log_error("Failed to bind UHCI device function: %s.\n",
		    str_error(rc));
		goto error;
	}

	fun_bound = true;

	rc = ddf_fun_add_to_category(instance->hc_fun, USB_HC_CATEGORY);
	if (rc != EOK) {
		usb_log_error("Failed to add UHCI to HC class: %s.\n",
		    str_error(rc));
		goto error;
	}

	rc = rh_init(instance->rh, instance->rh_fun, &regs, 0x10, 4);
	if (rc != EOK) {
		usb_log_error("Failed to setup UHCI root hub: %s.\n",
		    str_error(rc));
		goto error;
	}

	rc = ddf_fun_bind(instance->rh_fun);
	if (rc != EOK) {
		usb_log_error("Failed to register UHCI root hub: %s.\n",
		    str_error(rc));
		goto error;
	}

	return EOK;

error:
	if (fun_bound)
		ddf_fun_unbind(instance->hc_fun);
	if (hc_inited)
		hc_fini(&instance->hc);
	if (ih_registered)
		unregister_interrupt_handler(device, irq);
	if (instance->hc_fun != NULL)
		ddf_fun_destroy(instance->hc_fun);
	if (instance->rh_fun != NULL) {
		ddf_fun_destroy(instance->rh_fun);
	}
	return rc;
}
/**
 * @}
 */
