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
	rh_t rh;
} uhci_t;

static inline uhci_t * dev_to_uhci(const ddf_dev_t *dev)
{
	assert(dev);
	return dev->driver_data;
}

/** IRQ handling callback, forward status from call to diver structure.
 *
 * @param[in] dev DDF instance of the device to use.
 * @param[in] iid (Unused).
 * @param[in] call Pointer to the call from kernel.
 */
static void irq_handler(ddf_dev_t *dev, ipc_callid_t iid, ipc_call_t *call)
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
	assert(fun);
	ddf_fun_t *hc_fun = dev_to_uhci(fun->dev)->hc_fun;
	assert(hc_fun);

	if (handle != NULL)
		*handle = hc_fun->handle;
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
	assert(fun);
	rh_t *rh = fun->driver_data;
	assert(rh);
	return &rh->resource_list;
}

/** Interface to provide the root hub driver with hw info */
static hw_res_ops_t hw_res_iface = {
	.get_resource_list = get_resource_list,
	.enable_interrupt = NULL,
};

/** RH function support for uhci_rhd */
static ddf_dev_ops_t rh_ops = {
	.interfaces[USB_DEV_IFACE] = &usb_iface,
	.interfaces[HW_RES_DEV_IFACE] = &hw_res_iface
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
	if (!device)
		return EBADMEM;

	uhci_t *instance = ddf_dev_data_alloc(device, sizeof(uhci_t));
	if (instance == NULL) {
		usb_log_error("Failed to allocate OHCI driver.\n");
		return ENOMEM;
	}

#define CHECK_RET_DEST_FREE_RETURN(ret, message...) \
if (ret != EOK) { \
	if (instance->hc_fun) \
		instance->hc_fun->driver_data = NULL; \
		ddf_fun_destroy(instance->hc_fun); \
	if (instance->rh_fun) {\
		instance->rh_fun->driver_data = NULL; \
		ddf_fun_destroy(instance->rh_fun); \
	} \
	usb_log_error(message); \
	return ret; \
} else (void)0

	instance->rh_fun = NULL;
	instance->hc_fun = ddf_fun_create(device, fun_exposed, "uhci_hc");
	int ret = (instance->hc_fun == NULL) ? ENOMEM : EOK;
	CHECK_RET_DEST_FREE_RETURN(ret, "Failed to create UHCI HC function.\n");
	instance->hc_fun->ops = &hc_ops;
	instance->hc_fun->driver_data = &instance->hc.generic;

	instance->rh_fun = ddf_fun_create(device, fun_inner, "uhci_rh");
	ret = (instance->rh_fun == NULL) ? ENOMEM : EOK;
	CHECK_RET_DEST_FREE_RETURN(ret, "Failed to create UHCI RH function.\n");
	instance->rh_fun->ops = &rh_ops;
	instance->rh_fun->driver_data = &instance->rh;

	uintptr_t reg_base = 0;
	size_t reg_size = 0;
	int irq = 0;

	ret = get_my_registers(device, &reg_base, &reg_size, &irq);
	CHECK_RET_DEST_FREE_RETURN(ret,
	    "Failed to get I/O addresses for %" PRIun ": %s.\n",
	    device->handle, str_error(ret));
	usb_log_debug("I/O regs at 0x%p (size %zu), IRQ %d.\n",
	    (void *) reg_base, reg_size, irq);

	ret = disable_legacy(device);
	CHECK_RET_DEST_FREE_RETURN(ret,
	    "Failed to disable legacy USB: %s.\n", str_error(ret));

	const size_t ranges_count = hc_irq_pio_range_count();
	const size_t cmds_count = hc_irq_cmd_count();
	irq_pio_range_t irq_ranges[ranges_count];
	irq_cmd_t irq_cmds[cmds_count];
	ret = hc_get_irq_code(irq_ranges, sizeof(irq_ranges), irq_cmds,
	    sizeof(irq_cmds), reg_base, reg_size);
	CHECK_RET_DEST_FREE_RETURN(ret,
	    "Failed to generate IRQ commands: %s.\n", str_error(ret));

	irq_code_t irq_code = {
		.rangecount = ranges_count,
		.ranges = irq_ranges,
		.cmdcount = cmds_count,
		.cmds = irq_cmds
	};

        /* Register handler to avoid interrupt lockup */
        ret = register_interrupt_handler(device, irq, irq_handler, &irq_code);
        CHECK_RET_DEST_FREE_RETURN(ret,
            "Failed to register interrupt handler: %s.\n", str_error(ret));

	bool interrupts = false;
	ret = enable_interrupts(device);
	if (ret != EOK) {
		usb_log_warning("Failed to enable interrupts: %s."
		    " Falling back to polling.\n", str_error(ret));
	} else {
		usb_log_debug("Hw interrupts enabled.\n");
		interrupts = true;
	}

	ret = hc_init(&instance->hc, (void*)reg_base, reg_size, interrupts);
	CHECK_RET_DEST_FREE_RETURN(ret,
	    "Failed to init uhci_hcd: %s.\n", str_error(ret));

#define CHECK_RET_FINI_RETURN(ret, message...) \
if (ret != EOK) { \
	hc_fini(&instance->hc); \
	CHECK_RET_DEST_FREE_RETURN(ret, message); \
	return ret; \
} else (void)0

	ret = ddf_fun_bind(instance->hc_fun);
	CHECK_RET_FINI_RETURN(ret, "Failed to bind UHCI device function: %s.\n",
	    str_error(ret));

	ret = ddf_fun_add_to_category(instance->hc_fun, USB_HC_CATEGORY);
	CHECK_RET_FINI_RETURN(ret,
	    "Failed to add UHCI to HC class: %s.\n", str_error(ret));

	ret = rh_init(&instance->rh, instance->rh_fun,
	    (uintptr_t)instance->hc.registers + 0x10, 4);
	CHECK_RET_FINI_RETURN(ret,
	    "Failed to setup UHCI root hub: %s.\n", str_error(ret));

	ret = ddf_fun_bind(instance->rh_fun);
	CHECK_RET_FINI_RETURN(ret,
	    "Failed to register UHCI root hub: %s.\n", str_error(ret));

	return EOK;
#undef CHECK_RET_FINI_RETURN
}
/**
 * @}
 */
