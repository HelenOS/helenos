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
#include <usb/hub.h>
#include <usb/ddfiface.h>
#include <usb/usbdevice.h>

#include "hc.h"

static int dummy_reset(int foo, void *arg)
{
	hc_t *hc = (hc_t*)arg;
	assert(hc);
	hc->rh.address = 0;
	return EOK;
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
	device_keeper_init(&instance->manager);


	rh_init(&instance->rh, dev, instance->registers);
	/* TODO: implement */
	return EOK;
}
/*----------------------------------------------------------------------------*/
int hc_register_hub(hc_t *instance)
{
	async_usleep(1000000);
#define CHECK_RET_RETURN(ret, msg...) \
	if (ret != EOK) { \
		usb_log_error(msg); \
		return ret; \
	} else (void)0
	assert(instance);
	assert(instance->ddf_instance);
	assert(instance->ddf_instance->handle);
	ddf_dev_t *dev = instance->rh.device;
	int ret = EOK;

	usb_hc_connection_t conn;
	ret =
	    usb_hc_connection_initialize(&conn, instance->ddf_instance->handle);
	CHECK_RET_RETURN(ret, "Failed to initialize hc connection.\n");

	ret = usb_hc_connection_open(&conn);
	CHECK_RET_RETURN(ret, "Failed to open hc connection.\n");

	usb_address_t address;
	devman_handle_t handle;
	ret = usb_hc_new_device_wrapper(dev, &conn, USB_SPEED_FULL, dummy_reset,
	    0, instance, &address, &handle, NULL, NULL, NULL);
	CHECK_RET_RETURN(ret, "Failed to add rh device.\n");

	ret = usb_hc_connection_close(&conn);
	CHECK_RET_RETURN(ret, "Failed to close hc connection.\n");
	return EOK;
}
/*----------------------------------------------------------------------------*/
int hc_schedule(hc_t *instance, batch_t *batch)
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
void hc_interrupt(hc_t *instance, uint16_t status)
{
	assert(instance);
	/* TODO: Check for interrupt cause */
	rh_interrupt(&instance->rh);
	/* TODO: implement */
}
/**
 * @}
 */
