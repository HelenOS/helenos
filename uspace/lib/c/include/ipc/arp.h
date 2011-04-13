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
 * ARP module messages.
 * @see arp_interface.h
 */

#ifndef LIBC_ARP_MESSAGES_
#define LIBC_ARP_MESSAGES_

#include <ipc/net.h>

/** ARP module messages. */
typedef enum {
	/** Clean cache message.
	 * @see arp_clean_cache()
	 */
	NET_ARP_CLEAN_CACHE = NET_ARP_FIRST,
	/** Clear address cache message.
	 * @see arp_clear_address_msg()
	 */
	NET_ARP_CLEAR_ADDRESS,
	/** Clear device cache message.
	 * @see arp_clear_device_req()
	 */
	NET_ARP_CLEAR_DEVICE,
	/** New device message.
	 * @see arp_device_req()
	 */
	NET_ARP_DEVICE,
	/** Address translation message.
	 * @see arp_translate_req()
	 */
	NET_ARP_TRANSLATE
} arp_messages;

/** @name ARP specific message parameters definitions */
/*@{*/

/** Return the protocol service message parameter.
 *
 * @param[in] call Message call structure.
 *
 */
#define ARP_GET_NETIF(call) ((services_t) IPC_GET_ARG2(call))

/*@}*/

#endif

/** @}
 */
