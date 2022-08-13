/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup udp
 * @{
 */

/**
 * @file UDP message
 */

#include <io/log.h>
#include <stdlib.h>

#include "msg.h"
#include "udp_type.h"

/** Alocate new segment structure. */
udp_msg_t *udp_msg_new(void)
{
	return calloc(1, sizeof(udp_msg_t));
}

/** Delete segment. */
void udp_msg_delete(udp_msg_t *msg)
{
	free(msg->data);
	free(msg);
}

/**
 * @}
 */
