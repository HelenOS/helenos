/*
 * SPDX-FileCopyrightText: 2010 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup usbinfo
 * @{
 */
/** @file
 * Common header for usbinfo application.
 */
#ifndef USBINFO_USBINFO_H_
#define USBINFO_USBINFO_H_

#include <usb/usb.h>
#include <usb/descriptor.h>
#include <usb/dev/device.h>
#include <usb/dev/dp.h>

typedef struct {
	int opt;
	void (*action)(usb_device_t *);
	bool active;
} usbinfo_action_t;

#define NAME "usbinfo"

extern void dump_buffer(const char *, size_t, const uint8_t *, size_t);
extern const char *get_indent(size_t);
extern void dump_match_ids(match_id_list_t *, const char *);
extern void dump_usb_descriptor(uint8_t *, size_t);
extern void dump_descriptor_tree(uint8_t *, size_t);

static inline void internal_error(errno_t err)
{
	fprintf(stderr, NAME ": internal error (%s).\n", str_error(err));
}

typedef void (*dump_descriptor_in_tree_t)(const uint8_t *, size_t, void *);

extern void browse_descriptor_tree(uint8_t *, size_t,
    usb_dp_descriptor_nesting_t *, dump_descriptor_in_tree_t, size_t, void *);

extern void list(void);

extern void dump_short_device_identification(usb_device_t *);
extern void dump_device_match_ids(usb_device_t *);
extern void dump_descriptor_tree_brief(usb_device_t *);
extern void dump_descriptor_tree_full(usb_device_t *);
extern void dump_strings(usb_device_t *);
extern void dump_status(usb_device_t *);
extern void dump_hidreport_raw(usb_device_t *);
extern void dump_hidreport_usages(usb_device_t *);

#endif
/**
 * @}
 */
