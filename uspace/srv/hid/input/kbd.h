/*
 * SPDX-FileCopyrightText: 2006 Josef Cejka
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup inputgen generic
 * @brief HelenOS input server.
 * @ingroup input
 * @{
 */
/** @file
 */

#ifndef KBD_KBD_H_
#define KBD_KBD_H_

#include <adt/list.h>
#include <ipc/loc.h>

struct kbd_port_ops;
struct kbd_ctl_ops;
struct layout;

typedef struct kbd_dev {
	/** Link to kbd_devs list */
	link_t link;

	/** Service ID (only for kbdev devices) */
	service_id_t svc_id;

	/** Device service name (only for kbdev devices) */
	char *svc_name;

	/** Port ops */
	struct kbd_port_ops *port_ops;

	/** Ctl ops */
	struct kbd_ctl_ops *ctl_ops;

	/** Controller-private data */
	void *ctl_private;

	/** Currently active modifiers. */
	unsigned mods;

	/** Currently pressed lock keys. We track these to tackle autorepeat. */
	unsigned lock_keys;

	/** Active keyboard layout */
	struct layout *active_layout;
} kbd_dev_t;

extern void kbd_push_data(kbd_dev_t *, sysarg_t);
extern void kbd_push_event(kbd_dev_t *, int, unsigned int);

#endif

/**
 * @}
 */
