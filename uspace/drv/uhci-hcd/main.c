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
#include <device/hw_res.h>
#include <errno.h>
#include <str_error.h>

#include <usb_iface.h>
#include <usb/ddfiface.h>
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
static int uhci_add_device(ddf_dev_t *device)
{
	assert(device);
	uhci_t *hcd = NULL;
#define CHECK_RET_FREE_HC_RETURN(ret, message...) \
if (ret != EOK) { \
	usb_log_error(message); \
	if (hcd != NULL) \
		free(hcd); \
	return ret; \
}

	usb_log_info("uhci_add_device() called\n");

	uintptr_t io_reg_base = 0;
	size_t io_reg_size = 0;
	int irq = 0;

	int ret =
	    pci_get_my_registers(device, &io_reg_base, &io_reg_size, &irq);
	CHECK_RET_FREE_HC_RETURN(ret,
	    "Failed(%d) to get I/O addresses:.\n", ret, device->handle);
	usb_log_info("I/O regs at 0x%X (size %zu), IRQ %d.\n",
	    io_reg_base, io_reg_size, irq);

	ret = pci_disable_legacy(device);
	CHECK_RET_FREE_HC_RETURN(ret,
	    "Failed(%d) disable legacy USB: %s.\n", ret, str_error(ret));

#if 0
	ret = pci_enable_interrupts(device);
	if (ret != EOK) {
		usb_log_warning(
		    "Failed(%d) to enable interrupts, fall back to polling.\n",
		    ret);
	}
#endif

	hcd = malloc(sizeof(uhci_t));
	ret = (hcd != NULL) ? EOK : ENOMEM;
	CHECK_RET_FREE_HC_RETURN(ret,
	    "Failed(%d) to allocate memory for uhci hcd.\n", ret);

	ret = uhci_init(hcd, device, (void*)io_reg_base, io_reg_size);
	CHECK_RET_FREE_HC_RETURN(ret, "Failed(%d) to init uhci-hcd.\n",
	    ret);
#undef CHECK_RET_FREE_HC_RETURN

	/*
	 * We might free hcd, but that does not matter since no one
	 * else would access driver_data anyway.
	 */
	device->driver_data = hcd;

	ddf_fun_t *rh = NULL;
#define CHECK_RET_FINI_FREE_RETURN(ret, message...) \
if (ret != EOK) { \
	usb_log_error(message); \
	if (hcd != NULL) {\
		uhci_fini(hcd); \
		free(hcd); \
	} \
	if (rh != NULL) \
		free(rh); \
	return ret; \
}

	/* It does no harm if we register this on polling */
	ret = register_interrupt_handler(device, irq, irq_handler,
	    &hcd->interrupt_code);
	CHECK_RET_FINI_FREE_RETURN(ret,
	    "Failed(%d) to register interrupt handler.\n", ret);

	ret = setup_root_hub(&rh, device);
	CHECK_RET_FINI_FREE_RETURN(ret,
	    "Failed(%d) to setup UHCI root hub.\n", ret);
	rh->driver_data = hcd->ddf_instance;

	ret = ddf_fun_bind(rh);
	CHECK_RET_FINI_FREE_RETURN(ret,
	    "Failed(%d) to register UHCI root hub.\n", ret);

	return EOK;
#undef CHECK_RET_FINI_FREE_RETURN
}
/*----------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	sleep(3);
	usb_log_enable(USB_LOG_LEVEL_INFO, NAME);

	return ddf_driver_main(&uhci_driver);
}
/**
 * @}
 */
