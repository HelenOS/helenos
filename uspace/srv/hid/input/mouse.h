/*
 * Copyright (c) 2021 Jiri Svoboda
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
 * @brief Mouse device handling.
 * @ingroup input
 * @{
 */
/** @file
 */

#ifndef MOUSE_H_
#define MOUSE_H_

#include <adt/list.h>
#include <ipc/loc.h>

struct mouse_port_ops;
struct mouse_proto_ops;

typedef struct mouse_dev {
	/** Link to mouse_devs list */
	link_t link;

	/** Service ID (only for mousedev devices) */
	service_id_t svc_id;

	/** Device service name (only for mousedev devices) */
	char *svc_name;

	/** Port ops */
	struct mouse_port_ops *port_ops;

	/** Protocol ops */
	struct mouse_proto_ops *proto_ops;
} mouse_dev_t;

extern void mouse_push_data(mouse_dev_t *, sysarg_t);
extern void mouse_push_event_move(mouse_dev_t *, int, int, int);
extern void mouse_push_event_abs_move(mouse_dev_t *, unsigned int, unsigned int,
    unsigned int, unsigned int);
extern void mouse_push_event_button(mouse_dev_t *, int, int);
extern void mouse_push_event_dclick(mouse_dev_t *, int);

#endif

/**
 * @}
 */
