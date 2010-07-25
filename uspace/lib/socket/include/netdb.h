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

/** @addtogroup netdb
 *  @{
 */

/** @file
 *  Structures and interfaces according to the BSD netdb.h file.
 */

#ifndef __NET_NETDB_H__
#define __NET_NETDB_H__

#include <sys/types.h>

/** Structure returned by network data base library.
 *  All addresses are supplied in host order, and returned in network order (suitable for use in system calls).
 */
struct	hostent {
	/** Official host name.
	 */
	char * h_name;
	/** Alias list.
	 */
	char **	h_aliases;
	/** Host address type.
	 */
	int h_addrtype;
	/** Address length.
	 */
	int h_length;
	/** List of addresses from name server.
	 */
	char **	h_addr_list;
	/** Address, for backward compatiblity.
	 */
#define	h_addr	h_addr_list[0]
};

/** @name Host entry address types definitions.
 */
/*@{*/

/** Authoritative Answer Host not found address type.
 */
#define	HOST_NOT_FOUND	1

/** Non-Authoritive Host not found, or SERVERFAIL address type.
 */
#define	TRY_AGAIN	2

/** Non recoverable errors, FORMERR, REFUSED, NOTIMP address type.
 */
#define	NO_RECOVERY	3

/** Valid name, no data record of requested type address type.
 */
#define	NO_DATA		4

/** No address, look for MX record address type.
 */
#define	NO_ADDRESS	NO_DATA

/*@}*/

/** Returns host entry by the host address.
 *  @param[in] address The host address.
 *  @param[in] len The address length.
 *  @param[in] type The address type.
 *  @returns Host entry information.
 */
//struct hostent *	gethostbyaddr(const void * address, int len, int type);

/** Returns host entry by the host name.
 *  @param[in] name The host name.
 *  @returns Host entry information.
 */
//struct hostent *	gethostbyname(const char * name);

#endif

/** @}
 */
