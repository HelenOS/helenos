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

#include "pci.h"

#define NAME "ehci-hcd"

static int ehci_add_device(ddf_dev_t *device);
/*----------------------------------------------------------------------------*/
static driver_ops_t ehci_driver_ops = {
	.add_device = ehci_add_device,
};
/*----------------------------------------------------------------------------*/
static driver_t ehci_driver = {
	.name = NAME,
	.driver_ops = &ehci_driver_ops
};
/*----------------------------------------------------------------------------*/
static int ehci_add_device(ddf_dev_t *device)
{
	assert(device);
#define CHECK_RET_RETURN(ret, message...) \
if (ret != EOK) { \
	usb_log_error(message); \
	return ret; \
}

	usb_log_info("uhci_add_device() called\n");

	uintptr_t mem_reg_base = 0;
	size_t mem_reg_size = 0;
	int irq = 0;

	int ret =
	    pci_get_my_registers(device, &mem_reg_base, &mem_reg_size, &irq);
	CHECK_RET_RETURN(ret,
	    "Failed(%d) to get memory addresses:.\n", ret, device->handle);
	usb_log_info("Memory mapped regs at 0x%X (size %zu), IRQ %d.\n",
	    mem_reg_base, mem_reg_size, irq);

	ret = pci_disable_legacy(device);
	CHECK_RET_RETURN(ret,
	    "Failed(%d) disable legacy USB: %s.\n", ret, str_error(ret));

	return EOK;
#undef CHECK_RET_RETURN
}
/*----------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	usb_log_enable(USB_LOG_LEVEL_ERROR, NAME);
	return ddf_driver_main(&ehci_driver);
}
/**
 * @}
 */
