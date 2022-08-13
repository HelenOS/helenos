/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup udp
 * @{
 */
/** @file UDP message
 */

#ifndef MSG_H
#define MSG_H

#include "udp_type.h"

extern udp_msg_t *udp_msg_new(void);
extern void udp_msg_delete(udp_msg_t *);

#endif

/** @}
 */
