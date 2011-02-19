/*
 * Copyright (c) 2011 Vojtech Horky, Jan Vesely
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
/** @addtogroup usb
 * @{
 */
/** @file
 * @brief UHCI driver
 */
#include <driver.h>
#include <usb_iface.h>
#include <ipc/irc.h>
#include <ipc/ns.h>
#include <ipc/services.h>
#include <sysinfo.h>

#include <errno.h>

#include <usb/debug.h>

#include "iface.h"
#include "pci.h"
#include "root_hub.h"
#include "uhci.h"

#define NAME "uhci-hcd"

static int uhci_add_device(device_t *device);
static int usb_iface_get_hc_handle(device_t *dev, devman_handle_t *handle);
/*----------------------------------------------------------------------------*/
static int usb_iface_get_hc_handle(device_t *dev, devman_handle_t *handle)
{
	/* This shall be called only for the UHCI itself. */
	assert(dev->parent == NULL);

	*handle = dev->handle;
	return EOK;
}
/*----------------------------------------------------------------------------*/
static usb_iface_t hc_usb_iface = {
	.get_hc_handle = usb_iface_get_hc_handle
};
/*----------------------------------------------------------------------------*/
static device_ops_t uhci_ops = {
	.interfaces[USB_DEV_IFACE] = &hc_usb_iface,
	.interfaces[USBHC_DEV_IFACE] = &uhci_iface
};
/*----------------------------------------------------------------------------*/
static driver_ops_t uhci_driver_ops = {
	.add_device = uhci_add_device,
};
/*----------------------------------------------------------------------------*/
static driver_t uhci_driver = {
	.name = NAME,
	.driver_ops = &uhci_driver_ops
};
/*----------------------------------------------------------------------------*/
static void irq_handler(device_t *device, ipc_callid_t iid, ipc_call_t *call)
{
	assert(device);
	uhci_t *hc = dev_to_uhci(device);
	usb_log_info("LOL HARDWARE INTERRUPT: %p.\n", hc);
	uint16_t status = IPC_GET_ARG1(*call);
	assert(hc);
	uhci_interrupt(hc, status);
}
/*----------------------------------------------------------------------------*/
#define CHECK_RET_RETURN(ret, message...) \
if (ret != EOK) { \
	usb_log_error(message); \
	return ret; \
}

static int uhci_add_device(device_t *device)
{
	assert(device);

	usb_log_info("uhci_add_device() called\n");
	device->ops = &uhci_ops;

	uintptr_t io_reg_base;
	size_t io_reg_size;
	int irq;

	int ret =
	    pci_get_my_registers(device, &io_reg_base, &io_reg_size, &irq);

	CHECK_RET_RETURN(ret,
	    "Failed(%d) to get I/O registers addresses for device:.\n",
	    ret, device->handle);
	usb_log_info("I/O regs at 0x%X (size %zu), IRQ %d.\n",
	    io_reg_base, io_reg_size, irq);


  sysarg_t apic;
  sysarg_t i8259;
	int irc_phone = -1;
	int irc_service = 0;

  if ((sysinfo_get_value("apic", &apic) == EOK) && (apic)) {
    irc_service = SERVICE_APIC;
		usb_log_debug("SERVICE_APIC\n");
	} else if ((sysinfo_get_value("i8259", &i8259) == EOK) && (i8259)) {
    irc_service = SERVICE_I8259;
		usb_log_debug("SERVICE_I8259\n");
	}

  if (irc_service) {
    while (irc_phone < 0)
      irc_phone = service_connect_blocking(irc_service, 0, 0);
  }
	usb_log_debug("Interrupt conttroller phone: %d\n", irc_phone);

	async_msg_1(irc_phone, IRC_ENABLE_INTERRUPT, irq);
	async_hangup(irc_phone);


	uhci_t *uhci_hc = malloc(sizeof(uhci_t));
	ret = (uhci_hc != NULL) ? EOK : ENOMEM;
	CHECK_RET_RETURN(ret, "Failed to allocate memory for uhci hcd driver.\n");

	ret = uhci_init(uhci_hc, (void*)io_reg_base, io_reg_size);
	if (ret != EOK) {
		usb_log_error("Failed to init uhci-hcd.\n");
		free(uhci_hc);
		return ret;
	}

	ret = register_interrupt_handler(device, irq, irq_handler,
	    &uhci_hc->interrupt_code);
	if (ret != EOK) {
		usb_log_error("Failed to register interrupt handler.\n");
		uhci_fini(uhci_hc);
		free(uhci_hc);
		return ret;
	}

	device_t *rh;
	ret = setup_root_hub(&rh, device);
	if (ret != EOK) {
		usb_log_error("Failed to setup uhci root hub.\n");
		uhci_fini(uhci_hc);
		free(uhci_hc);
		return ret;
	}

	ret = child_device_register(rh, device);
	if (ret != EOK) {
		usb_log_error("Failed to register root hub.\n");
		uhci_fini(uhci_hc);
		free(uhci_hc);
		free(rh);
		return ret;
	}

	device->driver_data = uhci_hc;
	return EOK;
}
/*----------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	sleep(3);
	usb_log_enable(USB_LOG_LEVEL_DEBUG, NAME);

	return driver_main(&uhci_driver);
}
/**
 * @}
 */
