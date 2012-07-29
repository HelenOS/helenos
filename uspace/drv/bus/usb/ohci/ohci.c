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

static inline ohci_t * dev_to_ohci(ddf_dev_t *dev)
{
	assert(dev);
	return dev->driver_data;
}
/** IRQ handling callback, identifies device
 *
 * @param[in] dev DDF instance of the device to use.
 * @param[in] iid (Unused).
 * @param[in] call Pointer to the call that represents interrupt.
 */
static void irq_handler(ddf_dev_t *dev, ipc_callid_t iid, ipc_call_t *call)
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
		*address = dev_to_ohci(fun->dev)->hc.rh.address;
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
	ddf_fun_t *hc_fun = dev_to_ohci(fun->dev)->hc_fun;
	assert(hc_fun);

	if (handle != NULL)
		*handle = hc_fun->handle;
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
	if (device == NULL)
		return EBADMEM;

	ohci_t *instance = ddf_dev_data_alloc(device,sizeof(ohci_t));
	if (instance == NULL) {
		usb_log_error("Failed to allocate OHCI driver.\n");
		return ENOMEM;
	}

#define CHECK_RET_DEST_FREE_RETURN(ret, message...) \
if (ret != EOK) { \
	if (instance->hc_fun) { \
		instance->hc_fun->driver_data = NULL; \
		ddf_fun_destroy(instance->hc_fun); \
	} \
	if (instance->rh_fun) { \
		instance->rh_fun->driver_data = NULL; \
		ddf_fun_destroy(instance->rh_fun); \
	} \
	usb_log_error(message); \
	return ret; \
} else (void)0

	instance->hc_fun = ddf_fun_create(device, fun_exposed, "ohci_hc");
	int ret = instance->hc_fun ? EOK : ENOMEM;
	CHECK_RET_DEST_FREE_RETURN(ret,
	    "Failed to create OHCI HC function: %s.\n", str_error(ret));
	instance->hc_fun->ops = &hc_ops;
	instance->hc_fun->driver_data = &instance->hc;

	instance->rh_fun = ddf_fun_create(device, fun_inner, "ohci_rh");
	ret = instance->rh_fun ? EOK : ENOMEM;
	CHECK_RET_DEST_FREE_RETURN(ret,
	    "Failed to create OHCI RH function: %s.\n", str_error(ret));
	instance->rh_fun->ops = &rh_ops;

	uintptr_t reg_base = 0;
	size_t reg_size = 0;
	int irq = 0;

	ret = get_my_registers(device, &reg_base, &reg_size, &irq);
	CHECK_RET_DEST_FREE_RETURN(ret,
	    "Failed to get register memory addresses for %" PRIun ": %s.\n",
	    device->handle, str_error(ret));
	usb_log_debug("Memory mapped regs at %p (size %zu), IRQ %d.\n",
	    (void *) reg_base, reg_size, irq);

	const size_t ranges_count = hc_irq_pio_range_count();
	const size_t cmds_count = hc_irq_cmd_count();
	irq_pio_range_t irq_ranges[ranges_count];
	irq_cmd_t irq_cmds[cmds_count];
	irq_code_t irq_code = {
		.rangecount = ranges_count,
		.ranges = irq_ranges,
		.cmdcount = cmds_count,
		.cmds = irq_cmds
	};

	ret = hc_get_irq_code(irq_ranges, sizeof(irq_ranges), irq_cmds,
	    sizeof(irq_cmds), reg_base, reg_size);
	CHECK_RET_DEST_FREE_RETURN(ret,
	    "Failed to generate IRQ code: %s.\n", str_error(ret));


	/* Register handler to avoid interrupt lockup */
	ret = register_interrupt_handler(device, irq, irq_handler, &irq_code);
	CHECK_RET_DEST_FREE_RETURN(ret,
	    "Failed to register interrupt handler: %s.\n", str_error(ret));

	/* Try to enable interrupts */
	bool interrupts = false;
	ret = enable_interrupts(device);
	if (ret != EOK) {
		usb_log_warning("Failed to enable interrupts: %s."
		    " Falling back to polling\n", str_error(ret));
		/* We don't need that handler */
		unregister_interrupt_handler(device, irq);
	} else {
		usb_log_debug("Hw interrupts enabled.\n");
		interrupts = true;
	}

	ret = hc_init(&instance->hc, reg_base, reg_size, interrupts);
	CHECK_RET_DEST_FREE_RETURN(ret,
	    "Failed to init ohci_hcd: %s.\n", str_error(ret));

#define CHECK_RET_FINI_RETURN(ret, message...) \
if (ret != EOK) { \
	hc_fini(&instance->hc); \
	unregister_interrupt_handler(device, irq); \
	CHECK_RET_DEST_FREE_RETURN(ret, message); \
} else (void)0


	ret = ddf_fun_bind(instance->hc_fun);
	CHECK_RET_FINI_RETURN(ret,
	    "Failed to bind OHCI device function: %s.\n", str_error(ret));

	ret = ddf_fun_add_to_category(instance->hc_fun, USB_HC_CATEGORY);
	CHECK_RET_FINI_RETURN(ret,
	    "Failed to add OHCI to HC class: %s.\n", str_error(ret));

	ret = hc_register_hub(&instance->hc, instance->rh_fun);
	CHECK_RET_FINI_RETURN(ret,
	    "Failed to register OHCI root hub: %s.\n", str_error(ret));
	return ret;

#undef CHECK_RET_FINI_RETURN
}
/**
 * @}
 */
