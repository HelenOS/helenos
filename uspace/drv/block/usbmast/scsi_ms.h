/*
 * SPDX-FileCopyrightText: 2011 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup drvusbmast
 * @{
 */
/** @file
 * SCSI functions for USB mass storage.
 */

#ifndef USB_USBMAST_SCSI_MS_H_
#define USB_USBMAST_SCSI_MS_H_

#include <scsi/spc.h>
#include <stddef.h>
#include <stdint.h>
#include <usb/usb.h>
#include <usb/dev/driver.h>

/** Result of SCSI Inquiry command.
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
} usbmast_inquiry_data_t;

extern errno_t usbmast_inquiry(usbmast_fun_t *, usbmast_inquiry_data_t *);
extern errno_t usbmast_request_sense(usbmast_fun_t *, void *, size_t);
extern errno_t usbmast_read_capacity(usbmast_fun_t *, uint32_t *, uint32_t *);
extern errno_t usbmast_read(usbmast_fun_t *, uint64_t, size_t, void *);
extern errno_t usbmast_write(usbmast_fun_t *, uint64_t, size_t, const void *);
extern errno_t usbmast_sync_cache(usbmast_fun_t *, uint64_t, size_t);
extern const char *usbmast_scsi_dev_type_str(unsigned);

#endif

/**
 * @}
 */
