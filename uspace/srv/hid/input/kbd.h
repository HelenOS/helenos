/*
 * Copyright (c) 2006 Josef Cejka
 * Copyright (c) 2011 Jiri Svoboda
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
