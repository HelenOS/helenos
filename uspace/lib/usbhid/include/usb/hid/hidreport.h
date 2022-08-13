/*
 * SPDX-FileCopyrightText: 2011 Lubos Slovak
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libusbhid
 * @{
 */
/** @file
 * USB HID report parser initialization from descriptors.
 */

#ifndef LIBUSBHID_HIDREPORT_H_
#define LIBUSBHID_HIDREPORT_H_

#include <usb/dev/driver.h>
#include <usb/hid/hidparser.h>

/**
 * Retrieves the Report descriptor from the USB device and initializes the
 * report parser.
 *
 * \param[in] dev USB device representing a HID device.
 * \param[in/out] parser HID Report parser.
 * \param[out] report_desc Place to save report descriptor into.
 * \param[out] report_size
 *
 * \retval EOK if successful.
 * \retval EINVAL if one of the parameters is not given (is NULL).
 * \retval ENOENT if there are some descriptors missing.
 * \retval ENOMEM if an error with allocation occured.
 * \retval EINVAL if the Report descriptor's size does not match the size
 *         from the interface descriptor.
 * \return Other value inherited from function usb_pipe_start_session(),
 *         usb_pipe_end_session() or usb_request_get_descriptor().
 */
errno_t usb_hid_process_report_descriptor(usb_device_t *dev,
    usb_hid_report_t *report, uint8_t **report_desc, size_t *report_size);

#endif /* LIBUSB_HIDREPORT_H_ */

/**
 * @}
 */
