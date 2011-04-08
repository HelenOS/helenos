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
/** @addtogroup drvusbohcihc
 * @{
 */
/** @file
 * @brief OHCI Host controller driver routines
 */
#include <errno.h>
#include <str_error.h>
#include <adt/list.h>
#include <libarch/ddi.h>

#include <usb/debug.h>
#include <usb/usb.h>
#include <usb/ddfiface.h>
#include <usb/usbdevice.h>

#include "hc.h"

static int interrupt_emulator(hc_t *instance);
static void hc_gain_control(hc_t *instance);
static void hc_init_hw(hc_t *instance);
static int hc_init_transfer_lists(hc_t *instance);
/*----------------------------------------------------------------------------*/
int hc_register_hub(hc_t *instance, ddf_fun_t *hub_fun)
{
	assert(instance);
	assert(hub_fun);

	usb_address_t hub_address =
	    device_keeper_get_free_address(&instance->manager, USB_SPEED_FULL);
	instance->rh.address = hub_address;
	usb_device_keeper_bind(
	    &instance->manager, hub_address, hub_fun->handle);

	endpoint_t *ep = malloc(sizeof(endpoint_t));
	assert(ep);
	int ret = endpoint_init(ep, hub_address, 0, USB_DIRECTION_BOTH,
	    USB_TRANSFER_CONTROL, USB_SPEED_FULL, 64);
	assert(ret == EOK);
	ret = usb_endpoint_manager_register_ep(&instance->ep_manager, ep, 0);
	assert(ret == EOK);

	char *match_str = NULL;
	ret = asprintf(&match_str, "usb&class=hub");
//	ret = (match_str == NULL) ? ret : EOK;
	if (ret < 0) {
		usb_log_error(
		    "Failed(%d) to create root hub match-id string.\n", ret);
		return ret;
	}

	ret = ddf_fun_add_match_id(hub_fun, match_str, 100);
	if (ret != EOK) {
		usb_log_error("Failed add create root hub match-id.\n");
	}
	return ret;
}
/*----------------------------------------------------------------------------*/
int hc_init(hc_t *instance, ddf_fun_t *fun, ddf_dev_t *dev,
    uintptr_t regs, size_t reg_size, bool interrupts)
{
	assert(instance);
	int ret = EOK;
#define CHECK_RET_RETURN(ret, message...) \
if (ret != EOK) { \
	usb_log_error(message); \
	return ret; \
} else (void)0

	ret = pio_enable((void*)regs, reg_size, (void**)&instance->registers);
	CHECK_RET_RETURN(ret,
	    "Failed(%d) to gain access to device registers: %s.\n",
	    ret, str_error(ret));

	instance->ddf_instance = fun;
	usb_device_keeper_init(&instance->manager);
	ret = usb_endpoint_manager_init(&instance->ep_manager,
	    BANDWIDTH_AVAILABLE_USB11);
	CHECK_RET_RETURN(ret, "Failed to initialize endpoint manager: %s.\n",
	    ret, str_error(ret));

	if (!interrupts) {
		instance->interrupt_emulator =
		    fibril_create((int(*)(void*))interrupt_emulator, instance);
		fibril_add_ready(instance->interrupt_emulator);
	}

	hc_gain_control(instance);

	rh_init(&instance->rh, dev, instance->registers);

	hc_init_hw(instance);

	/* TODO: implement */
	return EOK;
}
/*----------------------------------------------------------------------------*/
int hc_schedule(hc_t *instance, usb_transfer_batch_t *batch)
{
	assert(instance);
	assert(batch);
	if (batch->target.address == instance->rh.address) {
		return rh_request(&instance->rh, batch);
	}
	/* TODO: implement */
	return ENOTSUP;
}
/*----------------------------------------------------------------------------*/
void hc_interrupt(hc_t *instance, uint32_t status)
{
	assert(instance);
	if (status == 0)
		return;
	if (status & IS_RHSC)
		rh_interrupt(&instance->rh);

	usb_log_info("OHCI interrupt: %x.\n", status);

	/* TODO: Check for further interrupt causes */
	/* TODO: implement */
}
/*----------------------------------------------------------------------------*/
int interrupt_emulator(hc_t *instance)
{
	assert(instance);
	usb_log_info("Started interrupt emulator.\n");
	while (1) {
		const uint32_t status = instance->registers->interrupt_status;
		instance->registers->interrupt_status = status;
		hc_interrupt(instance, status);
		async_usleep(1000);
	}
	return EOK;
}
/*----------------------------------------------------------------------------*/
void hc_gain_control(hc_t *instance)
{
	assert(instance);
	/* Interrupt routing enabled => smm driver is active */
	if (instance->registers->control & C_IR) {
		usb_log_info("Found SMM driver requesting ownership change.\n");
		instance->registers->command_status |= CS_OCR;
		while (instance->registers->control & C_IR) {
			async_usleep(1000);
		}
		usb_log_info("Ownership taken from SMM driver.\n");
		return;
	}

	const unsigned hc_status =
	    (instance->registers->control >> C_HCFS_SHIFT) & C_HCFS_MASK;
	/* Interrupt routing disabled && status != USB_RESET => BIOS active */
	if (hc_status != C_HCFS_RESET) {
		usb_log_info("Found BIOS driver.\n");
		if (hc_status == C_HCFS_OPERATIONAL) {
			usb_log_info("HC operational(BIOS).\n");
			return;
		}
		/* HC is suspended assert resume for 20ms */
		instance->registers->control &= (C_HCFS_RESUME << C_HCFS_SHIFT);
		async_usleep(20000);
		return;
	}

	/* HC is in reset (hw startup) => no other driver
	 * maintain reset for at least the time specified in USB spec (50 ms)*/
	async_usleep(50000);

	/* turn off legacy emulation */
	volatile uint32_t *ohci_emulation_reg =
	    (uint32_t*)((char*)instance->registers + 0x100);
	usb_log_info("OHCI legacy register status %p: %x.\n",
		ohci_emulation_reg, *ohci_emulation_reg);
	*ohci_emulation_reg = 0;

}
/*----------------------------------------------------------------------------*/
void hc_init_hw(hc_t *instance)
{
	assert(instance);
	const uint32_t fm_interval = instance->registers->fm_interval;
	instance->registers->command_status = CS_HCR;
	async_usleep(10);
	instance->registers->fm_interval = fm_interval;
	assert((instance->registers->command_status & CS_HCR) == 0);
	/* hc is now in suspend state */
	/* init queues */
	hc_init_transfer_lists(instance);
	/* TODO: init HCCA block */
	/* TODO: enable queues */
	/* TODO: enable interrupts */
	/* TODO: set periodic start to 90% */

	instance->registers->control &= (C_HCFS_OPERATIONAL << C_HCFS_SHIFT);
	usb_log_info("OHCI HC up and running.\n");
}
/*----------------------------------------------------------------------------*/
int hc_init_transfer_lists(hc_t *instance)
{
	assert(instance);
#define CHECK_RET_CLEAR_RETURN(ret, message...) \
	if (ret != EOK) { \
		usb_log_error(message); \
		transfer_list_fini(&instance->transfers_isochronous); \
		transfer_list_fini(&instance->transfers_interrupt); \
		transfer_list_fini(&instance->transfers_control); \
		transfer_list_fini(&instance->transfers_bulk); \
		return ret; \
	} else (void) 0

	int ret;
	ret = transfer_list_init(&instance->transfers_bulk, "BULK");
	CHECK_RET_CLEAR_RETURN(ret, "Failed to init BULK list.");

	ret = transfer_list_init(&instance->transfers_control, "CONTROL");
	CHECK_RET_CLEAR_RETURN(ret, "Failed to init CONTROL list.");

	ret = transfer_list_init(&instance->transfers_interrupt, "INTERRUPT");
	CHECK_RET_CLEAR_RETURN(ret, "Failed to init INTERRUPT list.");

	transfer_list_set_next(&instance->transfers_interrupt,
		&instance->transfers_isochronous);

	/* Assign pointers to be used during scheduling */
	instance->transfers[USB_TRANSFER_INTERRUPT] =
	  &instance->transfers_interrupt;
	instance->transfers[USB_TRANSFER_ISOCHRONOUS] =
	  &instance->transfers_interrupt;
	instance->transfers[USB_TRANSFER_CONTROL] =
	  &instance->transfers_control;
	instance->transfers[USB_TRANSFER_BULK] =
	  &instance->transfers_bulk;

	return EOK;
#undef CHECK_RET_CLEAR_RETURN
}
/**
 * @}
 */
