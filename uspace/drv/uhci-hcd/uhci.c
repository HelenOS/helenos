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
#include "iface.h"
#include "pci.h"


/** IRQ handling callback, identifies device
 *
 * @param[in] dev DDF instance of the device to use.
 * @param[in] iid (Unused).
 * @param[in] call Pointer to the call that represents interrupt.
 */
static void irq_handler(ddf_dev_t *dev, ipc_callid_t iid, ipc_call_t *call)
{
	assert(dev);
	hc_t *hc = &((uhci_t*)dev->driver_data)->hc;
	uint16_t status = IPC_GET_ARG1(*call);
	assert(hc);
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
	usb_device_keeper_t *manager = &((uhci_t*)fun->dev->driver_data)->hc.device_manager;

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
/** Gets handle of the respective hc (this or parent device).
 *
 * @param[in] root_hub_fun Root hub function seeking hc handle.
 * @param[out] handle Place to write the handle.
 * @return Error code.
 */
static int usb_iface_get_hc_handle(
    ddf_fun_t *fun, devman_handle_t *handle)
{
	assert(handle);
	ddf_fun_t *hc_fun = ((uhci_t*)fun->dev->driver_data)->hc_fun;
	assert(hc_fun != NULL);

	*handle = hc_fun->handle;
	return EOK;
}
/*----------------------------------------------------------------------------*/
/** This iface is generic for both RH and HC. */
static usb_iface_t usb_iface = {
	.get_hc_handle = usb_iface_get_hc_handle,
	.get_address = usb_iface_get_address
};
/*----------------------------------------------------------------------------*/
static ddf_dev_ops_t hc_ops = {
	.interfaces[USB_DEV_IFACE] = &usb_iface,
	.interfaces[USBHC_DEV_IFACE] = &hc_iface, /* see iface.h/c */
};
/*----------------------------------------------------------------------------*/
/** Get root hub hw resources (I/O registers).
 *
 * @param[in] fun Root hub function.
 * @return Pointer to the resource list used by the root hub.
 */
static hw_resource_list_t *get_resource_list(ddf_fun_t *fun)
{
	assert(fun);
	return &((uhci_rh_t*)fun->driver_data)->resource_list;
}
/*----------------------------------------------------------------------------*/
static hw_res_ops_t hw_res_iface = {
	.get_resource_list = get_resource_list,
	.enable_interrupt = NULL
};
/*----------------------------------------------------------------------------*/
static ddf_dev_ops_t uhci_rh_ops = {
	.interfaces[USB_DEV_IFACE] = &usb_iface,
	.interfaces[HW_RES_DEV_IFACE] = &hw_res_iface
};
/*----------------------------------------------------------------------------*/
/** Initialize hc and rh ddf structures and their respective drivers.
 *
 * @param[in] instance UHCI structure to use.
 * @param[in] device DDF instance of the device to use.
 *
 * This function does all the preparatory work for hc and rh drivers:
 *  - gets device hw resources
 *  - disables UHCI legacy support
 *  - asks for interrupt
 *  - registers interrupt handler
 */
int uhci_init(uhci_t *instance, ddf_dev_t *device)
{
	assert(instance);
	instance->hc_fun = NULL;
	instance->rh_fun = NULL;
#define CHECK_RET_DEST_FUN_RETURN(ret, message...) \
if (ret != EOK) { \
	usb_log_error(message); \
	if (instance->hc_fun) \
		ddf_fun_destroy(instance->hc_fun); \
	if (instance->rh_fun) \
		ddf_fun_destroy(instance->rh_fun); \
	return ret; \
}

	uintptr_t io_reg_base = 0;
	size_t io_reg_size = 0;
	int irq = 0;

	int ret =
	    pci_get_my_registers(device, &io_reg_base, &io_reg_size, &irq);
	CHECK_RET_DEST_FUN_RETURN(ret,
	    "Failed(%d) to get I/O addresses:.\n", ret, device->handle);
	usb_log_debug("I/O regs at 0x%X (size %zu), IRQ %d.\n",
	    io_reg_base, io_reg_size, irq);

	ret = pci_disable_legacy(device);
	CHECK_RET_DEST_FUN_RETURN(ret,
	    "Failed(%d) to disable legacy USB: %s.\n", ret, str_error(ret));

	bool interrupts = false;
	ret = pci_enable_interrupts(device);
	if (ret != EOK) {
		usb_log_warning(
		    "Failed(%d) to enable interrupts, fall back to polling.\n",
		    ret);
	} else {
		usb_log_debug("Hw interrupts enabled.\n");
		interrupts = true;
	}

	instance->hc_fun = ddf_fun_create(device, fun_exposed, "uhci-hc");
	ret = (instance->hc_fun == NULL) ? ENOMEM : EOK;
	CHECK_RET_DEST_FUN_RETURN(ret,
	    "Failed(%d) to create HC function.\n", ret);

	ret = hc_init(&instance->hc, instance->hc_fun,
	    (void*)io_reg_base, io_reg_size, interrupts);
	CHECK_RET_DEST_FUN_RETURN(ret, "Failed(%d) to init uhci-hcd.\n", ret);
	instance->hc_fun->ops = &hc_ops;
	instance->hc_fun->driver_data = &instance->hc;
	ret = ddf_fun_bind(instance->hc_fun);
	CHECK_RET_DEST_FUN_RETURN(ret,
	    "Failed(%d) to bind UHCI device function: %s.\n",
	    ret, str_error(ret));
#undef CHECK_RET_HC_RETURN

#define CHECK_RET_FINI_RETURN(ret, message...) \
if (ret != EOK) { \
	usb_log_error(message); \
	if (instance->hc_fun) \
		ddf_fun_destroy(instance->hc_fun); \
	if (instance->rh_fun) \
		ddf_fun_destroy(instance->rh_fun); \
	hc_fini(&instance->hc); \
	return ret; \
}

	/* It does no harm if we register this on polling */
	ret = register_interrupt_handler(device, irq, irq_handler,
	    &instance->hc.interrupt_code);
	CHECK_RET_FINI_RETURN(ret,
	    "Failed(%d) to register interrupt handler.\n", ret);

	instance->rh_fun = ddf_fun_create(device, fun_inner, "uhci-rh");
	ret = (instance->rh_fun == NULL) ? ENOMEM : EOK;
	CHECK_RET_FINI_RETURN(ret,
	    "Failed(%d) to create root hub function.\n", ret);

	ret = uhci_rh_init(&instance->rh, instance->rh_fun,
	    (uintptr_t)instance->hc.registers + 0x10, 4);
	CHECK_RET_FINI_RETURN(ret,
	    "Failed(%d) to setup UHCI root hub.\n", ret);

	instance->rh_fun->ops = &uhci_rh_ops;
	instance->rh_fun->driver_data = &instance->rh;
	ret = ddf_fun_bind(instance->rh_fun);
	CHECK_RET_FINI_RETURN(ret,
	    "Failed(%d) to register UHCI root hub.\n", ret);

	return EOK;
#undef CHECK_RET_FINI_RETURN
}
/**
 * @}
 */
