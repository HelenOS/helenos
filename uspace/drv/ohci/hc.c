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

	char *match_str = NULL;
	int ret = asprintf(&match_str, "usb&mid");
	ret = (match_str == NULL) ? ret : EOK;
	if (ret < 0) {
		usb_log_error("Failed to create root hub match-id string.\n");
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

	ret = pio_enable((void*)regs, reg_size, (void**)&instance->registers);
	if (ret != EOK) {
		usb_log_error("Failed to gain access to device registers.\n");
		return ret;
	}
	instance->ddf_instance = fun;
	usb_device_keeper_init(&instance->manager);

	if (!interrupts) {
		instance->interrupt_emulator =
		    fibril_create((int(*)(void*))interrupt_emulator, instance);
		fibril_add_ready(instance->interrupt_emulator);
	}

	rh_init(&instance->rh, dev, instance->registers);

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

	/* TODO: Check for further interrupt causes */
	/* TODO: implement */
}
/*----------------------------------------------------------------------------*/
int interrupt_emulator(hc_t *instance)
{
	assert(instance);
	usb_log_info("Started interrupt emulator.\n");
	while (1) {
		uint32_t status = instance->registers->interrupt_status;
		instance->registers->interrupt_status = status;
		hc_interrupt(instance, status);
		async_usleep(1000);
	}
	return EOK;
}
/**
 * @}
 */
