/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup inputgen generic
 * @brief Mouse protocol driver interface.
 * @ingroup input
 * @{
 */
/** @file
 */

#ifndef MOUSE_PROTO_H_
#define MOUSE_PROTO_H_

#include "mouse_port.h"

struct mouse_dev;

typedef struct mouse_proto_ops {
	void (*parse)(sysarg_t);
	errno_t (*init)(struct mouse_dev *);
} mouse_proto_ops_t;

extern mouse_proto_ops_t mousedev_proto;

#endif

/**
 * @}
 */
