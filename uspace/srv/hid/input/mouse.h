/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
