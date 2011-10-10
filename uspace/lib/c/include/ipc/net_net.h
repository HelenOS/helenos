/*
 * Copyright (c) 2009 Lukas Mejdrech
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

/** @addtogroup libc
 * @{
 */

/** @file
 * Networking subsystem central module messages.
 * @see net_interface.h
 */

#ifndef LIBC_NET_NET_MESSAGES_H_
#define LIBC_NET_NET_MESSAGES_H_

#include <ipc/net.h>

/** Networking subsystem central module messages. */
typedef enum {
	/** Return general configuration
	 * @see net_get_conf_req()
	 */
	NET_NET_GET_CONF = NET_FIRST,
	/** Return device specific configuration
	 * @see net_get_device_conf_req()
	 */
	NET_NET_GET_DEVICE_CONF,
	/** Return number of mastered devices */
	NET_NET_GET_DEVICES_COUNT,
	/** Return names and device IDs of all devices */
	NET_NET_GET_DEVICES,
	/** Notify the networking service about a ready device */
	NET_NET_DRIVER_READY
} net_messages;

#endif

/** @}
 */
