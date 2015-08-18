/*
 * Copyright (c) 2014 Martin Decky
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

/** @addtogroup genericipc
 * @{
 */
/**
 * @file interface.h
 * @brief List of all known interfaces and their codes.
 */

#ifndef ABI_IPC_INTERFACES_H_
#define ABI_IPC_INTERFACES_H_

#include <abi/fourcc.h>

#define IFACE_EXCHANGE_MASK  0x03
#define IFACE_MOD_MASK       0x04

/** Interface exchange management style
 *
 */
typedef enum {
	/** No explicit exchange management
	 *
	 * Suitable for protocols which use a single
	 * IPC message per exchange only.
	 */
	IFACE_EXCHANGE_ATOMIC = 0x00,
	
	/** Exchange management via mutual exclusion
	 *
	 * Suitable for any kind of client/server
	 * communication, but with possibly limited
	 * parallelism.
	 */
	IFACE_EXCHANGE_SERIALIZE = 0x01,
	
	/** Exchange management via connection cloning
	 *
	 * Suitable for servers which support client
	 * connection tracking and connection cloning.
	 */
	IFACE_EXCHANGE_PARALLEL = 0x02
} iface_exch_mgmt_t;

/** Interface modifiers
 *
 */
typedef enum {
	IFACE_MOD_NONE = 0x00,
	IFACE_MOD_CALLBACK = 0x04
} iface_mod_t;

typedef enum {
	INTERFACE_LOADER =
	    FOURCC_COMPACT('l', 'o', 'a', 'd') | IFACE_EXCHANGE_PARALLEL
} iface_t;

#endif

/** @}
 */
