/*
 * Copyright (c) 2015 Jan Kolarik
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

/** @file hw.c
 *
 * AR9271 hardware related functions implementation.
 *
 */

#include <usb/debug.h>
#include <unistd.h>

#include "hw.h"
#include "wmi.h"

/**
 * Try to wait for register value repeatedly until timeout defined by device 
 * is reached.
 * 
 * @param ar9271 Device structure.
 * @param offset Registry offset (address) to be read.
 * @param mask Mask to apply on read result.
 * @param value Required value we are waiting for.
 * 
 * @return EOK if succeed, ETIMEOUT on timeout, negative error code otherwise.
 */
static int hw_read_wait(ar9271_t *ar9271, uint32_t offset, uint32_t mask,
	uint32_t value)
{
	uint32_t result;
	
	for(size_t i = 0; i < HW_WAIT_LOOPS; i++) {
		udelay(HW_WAIT_TIME_US);

		int rc = wmi_reg_read(ar9271->htc_device, offset, &result);
		if(rc != EOK) {
			usb_log_debug("Failed to read register. Error %d\n",
				rc);
			continue;
		}

		if((result & mask) == value) {
			return EOK;
		}
	}
	
	return ETIMEOUT;
}

/**
 * Hardware reset of AR9271 device.
 * 
 * @param ar9271 Device structure.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
int hw_reset(ar9271_t *ar9271)
{
	reg_buffer_t buffer[] = {
		{
			.offset = AR9271_RTC_FORCE_WAKE,
			.value = AR9271_RTC_FORCE_WAKE_ENABLE | 
				AR9271_RTC_FORCE_WAKE_ON_INT
		},
		{
			.offset = AR9271_RC,
			.value = AR9271_RC_AHB
		},
		{
			.offset = AR9271_RTC_RESET,
			.value = 0
		}
	};
	
	int rc = wmi_reg_buffer_write(ar9271->htc_device, buffer,
		sizeof(buffer) / sizeof(reg_buffer_t));
	if(rc != EOK) {
		usb_log_error("Failed to set RT FORCE WAKE register.\n");
		return rc;
	}
	
	udelay(2);
	
	rc = wmi_reg_write(ar9271->htc_device, AR9271_RC, 0);
	if(rc != EOK) {
		usb_log_error("Failed to reset MAC AHB register.\n");
		return rc;
	}
	
	rc = wmi_reg_write(ar9271->htc_device, AR9271_RTC_RESET, 1);
	if(rc != EOK) {
		usb_log_error("Failed to bring up RTC register.\n");
		return rc;
	}
	
	rc = hw_read_wait(ar9271, 
		AR9271_RTC_STATUS, 
		AR9271_RTC_STATUS_MASK, 
		AR9271_RTC_STATUS_ON);
	if(rc != EOK) {
		usb_log_error("Failed to read wake up RTC register.\n");
		return rc;
	}
	
	/* TODO: Finish HW init. */
	
	return EOK;
}

/**
 * Initialize hardware of AR9271 device.
 * 
 * @param ar9271 Device structure.
 * 
 * @return EOK if succeed, negative error code otherwise.
 */
int hw_init(ar9271_t *ar9271)
{
	int rc = hw_reset(ar9271);
	if(rc != EOK) {
		usb_log_error("Failed to HW reset device.\n");
		return rc;
	}
	
	usb_log_info("HW initialization finished successfully.\n");
	
	return EOK;
}