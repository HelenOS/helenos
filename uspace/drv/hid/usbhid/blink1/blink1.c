/*
 * Copyright (c) 2014 Martin Decky
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

/** @addtogroup drvusbhid
 * @{
 */
/**
 * @file
 * USB blink(1) subdriver.
 */

#include <errno.h>
#include <str_error.h>
#include <usb/debug.h>
#include <ops/led_dev.h>
#include <usb/hid/request.h>
#include "blink1.h"

const char *HID_BLINK1_FUN_NAME = "blink1";
const char *HID_BLINK1_CATEGORY = "led";

#define BLINK1_REPORT_ID  0x0001

#define BLINK1_COMMAND_SET  0x006e

typedef struct {
	uint8_t id;
	uint8_t command;
	uint8_t arg0;
	uint8_t arg1;
	uint8_t arg2;
	uint8_t arg3;
	uint8_t arg4;
	uint8_t arg5;
} blink1_report_t;

static errno_t usb_blink1_color_set(ddf_fun_t *fun, pixel_t pixel)
{
	usb_blink1_t *blink1_dev = (usb_blink1_t *) ddf_fun_data_get(fun);
	if (blink1_dev == NULL) {
		usb_log_debug("Missing parameters.\n");
		return EINVAL;
	}
	
	blink1_report_t report;
	
	report.id = BLINK1_REPORT_ID;
	report.command = BLINK1_COMMAND_SET;
	report.arg0 = RED(pixel);
	report.arg1 = GREEN(pixel);
	report.arg2 = BLUE(pixel);
	report.arg3 = 0;
	report.arg4 = 0;
	report.arg5 = 0;
	
	return usbhid_req_set_report(
	    usb_device_get_default_pipe(blink1_dev->hid_dev->usb_dev),
	    usb_device_get_iface_number(blink1_dev->hid_dev->usb_dev),
	    USB_HID_REPORT_TYPE_FEATURE, (uint8_t *) &report, sizeof(report));
}

static led_dev_ops_t usb_blink1_iface = {
	.color_set = usb_blink1_color_set
};

static ddf_dev_ops_t blink1_ops = {
	.interfaces[LED_DEV_IFACE] = &usb_blink1_iface
};

errno_t usb_blink1_init(usb_hid_dev_t *hid_dev, void **data)
{
	if (hid_dev == NULL) {
		usb_log_error("Failed to init blink(1) structure: no structure "
		    "given.\n");
		return EINVAL;
	}
	
	/* Create the exposed function. */
	ddf_fun_t *fun = usb_device_ddf_fun_create(hid_dev->usb_dev,
	    fun_exposed, HID_BLINK1_FUN_NAME);
	if (fun == NULL) {
		usb_log_error("Could not create DDF function node `%s'.\n",
		    HID_BLINK1_FUN_NAME);
		return ENOMEM;
	}
	
	usb_blink1_t *blink1_dev = (usb_blink1_t *)
	    ddf_fun_data_alloc(fun, sizeof(usb_blink1_t));
	if (blink1_dev == NULL) {
		usb_log_error("Error while creating USB/HID blink(1) device "
		    "structure.\n");
		ddf_fun_destroy(fun);
		return ENOMEM;
	}
	
	ddf_fun_set_ops(fun, &blink1_ops);
	
	errno_t rc = ddf_fun_bind(fun);
	if (rc != EOK) {
		usb_log_error("Could not bind DDF function `%s': %s.\n",
		    ddf_fun_get_name(fun), str_error(rc));
		ddf_fun_destroy(fun);
		return rc;
	}
	
	rc = ddf_fun_add_to_category(fun, HID_BLINK1_CATEGORY);
	if (rc != EOK) {
		usb_log_error("Could not add DDF function to category %s: %s.\n",
		    HID_BLINK1_CATEGORY, str_error(rc));
		
		rc = ddf_fun_unbind(fun);
		if (rc != EOK) {
			usb_log_error("Could not unbind function `%s', it "
			    "will not be destroyed.\n", ddf_fun_get_name(fun));
			return rc;
		}
		
		ddf_fun_destroy(fun);
		return rc;
	}
	
	blink1_dev->fun = fun;
	blink1_dev->hid_dev = hid_dev;
	*data = blink1_dev;
	
	return EOK;
}

void usb_blink1_deinit(usb_hid_dev_t *hid_dev, void *data)
{
	if (data == NULL)
		return;
	
	usb_blink1_t *blink1_dev = (usb_blink1_t *) data;
	
	errno_t rc = ddf_fun_unbind(blink1_dev->fun);
	if (rc != EOK) {
		usb_log_error("Could not unbind function `%s', it "
		    "will not be destroyed.\n", ddf_fun_get_name(blink1_dev->fun));
		return;
	}
	
	ddf_fun_destroy(blink1_dev->fun);
}

/**
 * @}
 */
