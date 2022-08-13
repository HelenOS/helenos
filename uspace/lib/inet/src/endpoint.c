/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libinet
 * @{
 */
/** @file Internet endpoint
 */

#include <inet/endpoint.h>
#include <mem.h>

/** Initialize endpoint structure.
 *
 * Sets any address, any port number.
 *
 * @param ep Endpoint
 */
void inet_ep_init(inet_ep_t *ep)
{
	memset(ep, 0, sizeof(*ep));
}

/** Initialize endpoint pair structure.
 *
 * Sets any address, any port number for both local and remote sides.
 *
 * @param ep2 Endpoint pair
 */
void inet_ep2_init(inet_ep2_t *ep2)
{
	memset(ep2, 0, sizeof(*ep2));
}

/** @}
 */
