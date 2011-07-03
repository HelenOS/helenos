/*
 * Copyright (c) 2011 Vojtech Horky
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

/** @addtogroup drvusbmast
 * @{
 */
/** @file
 * Generic functions for USB mass storage.
 */

#ifndef USB_USBMAST_MAST_H_
#define USB_USBMAST_MAST_H_

#include <scsi/spc.h>
#include <sys/types.h>
#include <usb/usb.h>
#include <usb/dev/pipes.h>
#include <usb/dev/driver.h>

#define BULK_IN_EP 0
#define BULK_OUT_EP 1

/** Result of SCSI INQUIRY command.
 * This is already parsed structure, not the original buffer returned by
 * the device.
 */
typedef struct {
	/** SCSI peripheral device type */
	unsigned device_type;
	/** Whether the device is removable */
	bool removable;
	/** Vendor ID string */
	char vendor[SCSI_INQ_VENDOR_STR_BUFSIZE];
	/** Product ID string */
	char product[SCSI_INQ_PRODUCT_STR_BUFSIZE];
	/** Revision string */
	char revision[SCSI_INQ_REVISION_STR_BUFSIZE];
} usb_massstor_inquiry_result_t;

int usb_massstor_data_in(usb_device_t *dev, uint32_t, uint8_t, void *,
    size_t, void *, size_t, size_t *);
int usb_massstor_reset(usb_device_t *);
void usb_massstor_reset_recovery(usb_device_t *);
int usb_massstor_get_max_lun(usb_device_t *);
size_t usb_masstor_get_lun_count(usb_device_t *);
int usb_massstor_inquiry(usb_device_t *, usb_massstor_inquiry_result_t *);
const char *usb_str_masstor_scsi_peripheral_device_type(unsigned);

#endif

/**
 * @}
 */
