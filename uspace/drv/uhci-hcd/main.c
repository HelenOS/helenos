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
#include <ddf/driver.h>
#include <ddf/interrupt.h>
#include <usb_iface.h>
#include <usb/ddfiface.h>
#include <device/hw_res.h>

#include <errno.h>

#include <usb/debug.h>

#include "iface.h"
#include "pci.h"
#include "root_hub.h"
#include "uhci.h"

#define NAME "uhci-hcd"

static int uhci_add_device(ddf_dev_t *device);

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
static void irq_handler(ddf_dev_t *dev, ipc_callid_t iid, ipc_call_t *call)
{
	assert(dev);
	uhci_t *hc = dev_to_uhci(dev);
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

static int uhci_add_device(ddf_dev_t *device)
{
	assert(device);

	usb_log_info("uhci_add_device() called\n");

	uintptr_t io_reg_base = 0;
	size_t io_reg_size = 0;
	int irq = 0;

	int ret =
	    pci_get_my_registers(device, &io_reg_base, &io_reg_size, &irq);

	CHECK_RET_RETURN(ret,
	    "Failed(%d) to get I/O addresses:.\n", ret, device->handle);
	usb_log_info("I/O regs at 0x%X (size %zu), IRQ %d.\n",
	    io_reg_base, io_reg_size, irq);
	io_reg_size = 32;

//	ret = pci_enable_interrupts(device);
//	CHECK_RET_RETURN(ret, "Failed(%d) to get enable interrupts:\n", ret);

	uhci_t *uhci_hc = malloc(sizeof(uhci_t));
	ret = (uhci_hc != NULL) ? EOK : ENOMEM;
	CHECK_RET_RETURN(ret, "Failed to allocate memory for uhci hcd driver.\n");

	ret = uhci_init(uhci_hc, device, (void*)io_reg_base, io_reg_size);
	if (ret != EOK) {
		usb_log_error("Failed to init uhci-hcd.\n");
		free(uhci_hc);
		return ret;
	}

	/*
	 * We might free uhci_hc, but that does not matter since no one
	 * else would access driver_data anyway.
	 */
	device->driver_data = uhci_hc;
	ret = register_interrupt_handler(device, irq, irq_handler,
	    &uhci_hc->interrupt_code);
	if (ret != EOK) {
		usb_log_error("Failed to register interrupt handler.\n");
		uhci_fini(uhci_hc);
		free(uhci_hc);
		return ret;
	}

	ddf_fun_t *rh;
	ret = setup_root_hub(&rh, device);
	if (ret != EOK) {
		usb_log_error("Failed to setup uhci root hub.\n");
		uhci_fini(uhci_hc);
		free(uhci_hc);
		return ret;
	}
	rh->driver_data = uhci_hc->ddf_instance;

	ret = ddf_fun_bind(rh);
	if (ret != EOK) {
		usb_log_error("Failed to register root hub.\n");
		uhci_fini(uhci_hc);
		free(uhci_hc);
		free(rh);
		return ret;
	}

	return EOK;
}
/*----------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	sleep(3);
	usb_log_enable(USB_LOG_LEVEL_DEBUG, NAME);

	return ddf_driver_main(&uhci_driver);
}
/**
 * @}
 */
