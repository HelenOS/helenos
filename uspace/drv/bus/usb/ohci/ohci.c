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
#include "iface.h"
#include "pci.h"
#include "hc.h"
#include "root_hub.h"

typedef struct ohci {
	ddf_fun_t *hc_fun;
	ddf_fun_t *rh_fun;

	hc_t hc;
	rh_t rh;
} ohci_t;

static inline ohci_t * dev_to_ohci(ddf_dev_t *dev)
{
	assert(dev);
	assert(dev->driver_data);
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
	hc_t *hc = &dev_to_ohci(dev)->hc;
	assert(hc);
	const uint16_t status = IPC_GET_ARG1(*call);
	hc_interrupt(hc, status);
}
/*----------------------------------------------------------------------------*/
/** Get address of the device identified by handle.
 *
 * @param[in] dev DDF instance of the device to use.
 * @param[in] iid (Unused).
 * @param[in] call Pointer to the call that represents interrupt.
 */
static int usb_iface_get_address(
    ddf_fun_t *fun, devman_handle_t handle, usb_address_t *address)
{
	assert(fun);
	usb_device_keeper_t *manager = &dev_to_ohci(fun->dev)->hc.manager;

	usb_address_t addr = usb_device_keeper_find(manager, handle);
	if (addr < 0) {
		return addr;
	}

	if (address != NULL) {
		*address = addr;
	}

	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Gets handle of the respective hc (this device, hc function).
 *
 * @param[in] root_hub_fun Root hub function seeking hc handle.
 * @param[out] handle Place to write the handle.
 * @return Error code.
 */
static int usb_iface_get_hc_handle(
    ddf_fun_t *fun, devman_handle_t *handle)
{
	assert(fun);
	ddf_fun_t *hc_fun = dev_to_ohci(fun->dev)->hc_fun;
	assert(hc_fun);

	if (handle != NULL)
		*handle = hc_fun->handle;
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** Root hub USB interface */
static usb_iface_t usb_iface = {
	.get_hc_handle = usb_iface_get_hc_handle,
	.get_address = usb_iface_get_address
};
/*----------------------------------------------------------------------------*/
/** Standard USB HC options (HC interface) */
static ddf_dev_ops_t hc_ops = {
	.interfaces[USBHC_DEV_IFACE] = &hc_iface, /* see iface.h/c */
};
/*----------------------------------------------------------------------------*/
/** Standard USB RH options (RH interface) */
static ddf_dev_ops_t rh_ops = {
	.interfaces[USB_DEV_IFACE] = &usb_iface,
};
/*----------------------------------------------------------------------------*/
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
	ohci_t *instance = malloc(sizeof(ohci_t));
	if (instance == NULL) {
		usb_log_error("Failed to allocate OHCI driver.\n");
		return ENOMEM;
	}

#define CHECK_RET_DEST_FREE_RETURN(ret, message...) \
if (ret != EOK) { \
	if (instance->hc_fun) { \
		instance->hc_fun->ops = NULL; \
		instance->hc_fun->driver_data = NULL; \
		ddf_fun_destroy(instance->hc_fun); \
	} \
	if (instance->rh_fun) { \
		instance->rh_fun->ops = NULL; \
		instance->rh_fun->driver_data = NULL; \
		ddf_fun_destroy(instance->rh_fun); \
	} \
	free(instance); \
	usb_log_error(message); \
	return ret; \
} else (void)0

	instance->rh_fun = NULL;
	instance->hc_fun = ddf_fun_create(device, fun_exposed, "ohci_hc");
	int ret = instance->hc_fun ? EOK : ENOMEM;
	CHECK_RET_DEST_FREE_RETURN(ret, "Failed to create OHCI HC function.\n");
	instance->hc_fun->ops = &hc_ops;
	instance->hc_fun->driver_data = &instance->hc;

	instance->rh_fun = ddf_fun_create(device, fun_inner, "ohci_rh");
	ret = instance->rh_fun ? EOK : ENOMEM;
	CHECK_RET_DEST_FREE_RETURN(ret, "Failed to create OHCI RH function.\n");
	instance->rh_fun->ops = &rh_ops;

	uintptr_t reg_base = 0;
	size_t reg_size = 0;
	int irq = 0;

	ret = pci_get_my_registers(device, &reg_base, &reg_size, &irq);
	CHECK_RET_DEST_FREE_RETURN(ret,
	    "Failed to get memory addresses for %" PRIun ": %s.\n",
	    device->handle, str_error(ret));
	usb_log_debug("Memory mapped regs at %p (size %zu), IRQ %d.\n",
	    (void *) reg_base, reg_size, irq);

	bool interrupts = false;
#ifdef CONFIG_USBHC_NO_INTERRUPTS
	usb_log_warning("Interrupts disabled in OS config, "
	    "falling back to polling.\n");
#else
	ret = pci_enable_interrupts(device);
	if (ret != EOK) {
		usb_log_warning("Failed to enable interrupts: %s.\n",
		    str_error(ret));
		usb_log_info("HW interrupts not available, "
		    "falling back to polling.\n");
	} else {
		usb_log_debug("Hw interrupts enabled.\n");
		interrupts = true;
	}
#endif

	ret = hc_init(&instance->hc, reg_base, reg_size, interrupts);
	CHECK_RET_DEST_FREE_RETURN(ret, "Failed(%d) to init ohci_hcd.\n", ret);

#define CHECK_RET_FINI_RETURN(ret, message...) \
if (ret != EOK) { \
	hc_fini(&instance->hc); \
	CHECK_RET_DEST_FREE_RETURN(ret, message); \
} else (void)0

	/* It does no harm if we register this on polling */
	ret = register_interrupt_handler(device, irq, irq_handler,
	    &instance->hc.interrupt_code);
	CHECK_RET_FINI_RETURN(ret,
	    "Failed(%d) to register interrupt handler.\n", ret);

	ret = ddf_fun_bind(instance->hc_fun);
	CHECK_RET_FINI_RETURN(ret,
	    "Failed(%d) to bind OHCI device function: %s.\n",
	    ret, str_error(ret));

	ret = ddf_fun_add_to_class(instance->hc_fun, USB_HC_DDF_CLASS_NAME);
	CHECK_RET_FINI_RETURN(ret,
	    "Failed to add OHCI to HC class: %s.\n", str_error(ret));

	device->driver_data = instance;

	hc_start_hw(&instance->hc);
	hc_register_hub(&instance->hc, instance->rh_fun);
	return EOK;

#undef CHECK_RET_DEST_FUN_RETURN
#undef CHECK_RET_FINI_RETURN
}
/**
 * @}
 */
