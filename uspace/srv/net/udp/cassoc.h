/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup udp
 * @{
 */
/** @file UDP client associations
 */

#ifndef CASSOC_H
#define CASSOC_H

#include <errno.h>
#include "udp_type.h"

errno_t udp_cassoc_create(udp_client_t *, udp_assoc_t *, udp_cassoc_t **);
void udp_cassoc_destroy(udp_cassoc_t *);
errno_t udp_cassoc_get(udp_client_t *, sysarg_t, udp_cassoc_t **);
errno_t udp_cassoc_queue_msg(udp_cassoc_t *, inet_ep2_t *, udp_msg_t *);

#endif

/** @}
 */
