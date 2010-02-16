/*
 * Copyright (c) 2008 Lukas Mejdrech
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

/** @addtogroup icmp
 *  @{
 */

/** @file
 *  ICMP module.
 */

#ifndef __NET_ICMP_H__
#define __NET_ICMP_H__

#include <fibril_synch.h>

#include "../../include/icmp_codes.h"

#include "../../structures/int_map.h"

#include "icmp_header.h"

/** Type definition of the ICMP reply data.
 *  @see icmp_reply
 */
typedef struct icmp_reply	icmp_reply_t;

/** Type definition of the ICMP reply data pointer.
 *  @see icmp_reply
 */
typedef icmp_reply_t *	icmp_reply_ref;

/** Type definition of the ICMP global data.
 *  @see icmp_globals
 */
typedef struct icmp_globals	icmp_globals_t;

/** Pending replies map.
 *  Maps message identifiers to the pending replies.
 *  Sending fibril waits for its associated reply event.
 *  Receiving fibril sets the associated reply with the return value and signals the event.
 */
INT_MAP_DECLARE( icmp_replies, icmp_reply_t );

/** Echo specific data map.
 *  The bundle module gets an identifier of the assigned echo specific data while connecting.
 *  The identifier is used in the future semi-remote calls instead of the ICMP phone.
 */
INT_MAP_DECLARE( icmp_echo_data, icmp_echo_t );

/** ICMP reply data.
 */
struct icmp_reply{
	/** Reply result.
	 */
	int					result;
	/** Safety lock.
	 */
	fibril_mutex_t		mutex;
	/** Received or timeouted reply signaling.
	 */
	fibril_condvar_t	condvar;
};

/** ICMP global data.
 */
struct	icmp_globals{
	/** IP module phone.
	 */
	int				ip_phone;
	/** Reserved packet prefix length.
	 */
	size_t			prefix;
	/** Maximal packet content length.
	 */
	size_t			content;
	/** Reserved packet suffix length.
	 */
	size_t			suffix;
	/** Packet address length.
	 */
	size_t			addr_len;
	/** Networking module phone.
	 */
	int				net_phone;
	/** Indicates whether ICMP error reporting is enabled.
	 */
	int				error_reporting;
	/** Indicates whether ICMP echo replying (ping) is enabled.
	 */
	int				echo_replying;
	/** The last used identifier number.
	 */
	icmp_param_t	last_used_id;
	/** The budled modules assigned echo specific data.
	 */
	icmp_echo_data_t	echo_data;
	/** Echo timeout locks.
	 */
	icmp_replies_t	replies;
	/** Safety lock.
	 */
	fibril_rwlock_t	lock;
};

#endif

/** @}
 */

