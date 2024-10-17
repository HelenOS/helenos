/*
 * Copyright (c) 2024 Jiri Svoboda
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

/** @addtogroup abi_generic
 * @{
 */
/**
 * @file interface.h
 * @brief List of all known interfaces and their codes.
 */

#ifndef _ABI_IPC_INTERFACES_H_
#define _ABI_IPC_INTERFACES_H_

#include <abi/fourcc.h>

enum {
	IFACE_EXCHANGE_MASK = 0x03,
	IFACE_MOD_MASK      = 0x04,
};

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
	INTERFACE_ANY = 0,
	INTERFACE_LOADER =
	    FOURCC_COMPACT('l', 'o', 'a', 'd') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_PAGER =
	    FOURCC_COMPACT('p', 'a', 'g', 'e') | IFACE_EXCHANGE_ATOMIC,
	INTERFACE_LOGGER_WRITER =
	    FOURCC_COMPACT('l', 'o', 'g', 'w') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_LOGGER_CONTROL =
	    FOURCC_COMPACT('l', 'o', 'g', 'c') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_CORECFG =
	    FOURCC_COMPACT('c', 'c', 'f', 'g') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_FS =
	    FOURCC_COMPACT('f', 's', ' ', ' ') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_VFS =
	    FOURCC_COMPACT('v', 'f', 's', ' ') | IFACE_EXCHANGE_PARALLEL,
	INTERFACE_VFS_DRIVER =
	    FOURCC_COMPACT('v', 'f', 's', 'd') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_VFS_DRIVER_CB =
	    FOURCC_COMPACT('v', 'f', 's', 'd') | IFACE_EXCHANGE_PARALLEL | IFACE_MOD_CALLBACK,
	INTERFACE_BLOCK =
	    FOURCC_COMPACT('b', 'l', 'd', 'v') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_BLOCK_CB =
	    FOURCC_COMPACT('b', 'l', 'd', 'v') | IFACE_EXCHANGE_SERIALIZE | IFACE_MOD_CALLBACK,
	INTERFACE_CONSOLE =
	    FOURCC_COMPACT('c', 'o', 'n', 's') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_INPUT =
	    FOURCC_COMPACT('i', 'n', 'd', 'v') | IFACE_EXCHANGE_ATOMIC,
	INTERFACE_INPUT_CB =
	    FOURCC_COMPACT('i', 'n', 'd', 'v') | IFACE_EXCHANGE_SERIALIZE | IFACE_MOD_CALLBACK,
	INTERFACE_OUTPUT =
	    FOURCC_COMPACT('o', 'u', 'd', 'v') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_HOUND =
	    FOURCC_COMPACT('h', 'o', 'u', 'n') | IFACE_EXCHANGE_PARALLEL,
	INTERFACE_LOC_SUPPLIER =
	    FOURCC_COMPACT('l', 'o', 'c', 's') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_LOC_CONSUMER =
	    FOURCC_COMPACT('l', 'o', 'c', 'c') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_LOC_CB =
	    FOURCC_COMPACT('l', 'o', 'c', ' ') | IFACE_EXCHANGE_SERIALIZE | IFACE_MOD_CALLBACK,
	INTERFACE_DEVMAN_DEVICE =
	    FOURCC_COMPACT('d', 'v', 'd', 'v') | IFACE_EXCHANGE_PARALLEL,
	INTERFACE_DEVMAN_PARENT =
	    FOURCC_COMPACT('d', 'v', 'p', 't') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_IRC =
	    FOURCC_COMPACT('i', 'r', 'c', ' ') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_DDF =
	    FOURCC_COMPACT('d', 'd', 'f', ' ') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_DDF_DEVMAN =
	    FOURCC_COMPACT('d', 'd', 'f', 'm') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_DDF_CLIENT =
	    FOURCC_COMPACT('d', 'd', 'f', 'c') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_DDF_DRIVER =
	    FOURCC_COMPACT('d', 'd', 'f', 'd') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_INET =
	    FOURCC_COMPACT('i', 'n', 'e', 't') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_INET_CB =
	    FOURCC_COMPACT('i', 'n', 'e', 't') | IFACE_EXCHANGE_SERIALIZE | IFACE_MOD_CALLBACK,
	INTERFACE_INETCFG =
	    FOURCC_COMPACT('i', 'c', 'f', 'g') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_INETPING =
	    FOURCC_COMPACT('i', 'p', 'n', 'g') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_INETPING_CB =
	    FOURCC_COMPACT('i', 'p', 'n', 'g') | IFACE_EXCHANGE_SERIALIZE | IFACE_MOD_CALLBACK,
	INTERFACE_DHCP =
	    FOURCC_COMPACT('d', 'h', 'c', 'p') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_DNSR =
	    FOURCC_COMPACT('d', 'n', 's', 'r') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_IPLINK =
	    FOURCC_COMPACT('i', 'p', 'l', 'k') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_IPLINK_CB =
	    FOURCC_COMPACT('i', 'p', 'l', 'k') | IFACE_EXCHANGE_SERIALIZE | IFACE_MOD_CALLBACK,
	INTERFACE_TCP =
	    FOURCC_COMPACT('t', 'c', 'p', ' ') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_TCP_CB =
	    FOURCC_COMPACT('t', 'c', 'p', ' ') | IFACE_EXCHANGE_SERIALIZE | IFACE_MOD_CALLBACK,
	INTERFACE_UDP =
	    FOURCC_COMPACT('u', 'd', 'p', ' ') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_UDP_CB =
	    FOURCC_COMPACT('u', 'd', 'p', ' ') | IFACE_EXCHANGE_SERIALIZE | IFACE_MOD_CALLBACK,
	INTERFACE_CLIPBOARD =
	    FOURCC_COMPACT('c', 'l', 'i', 'p') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_CHAR_CB =
	    FOURCC_COMPACT('b', 'l', 'd', 'v') | IFACE_EXCHANGE_PARALLEL | IFACE_MOD_CALLBACK,
	INTERFACE_AUDIO_PCM_CB =
	    FOURCC_COMPACT('a', 'p', 'c', 'm') | IFACE_EXCHANGE_PARALLEL | IFACE_MOD_CALLBACK,
	INTERFACE_NIC_CB =
	    FOURCC_COMPACT('n', 'i', 'c', ' ') | IFACE_EXCHANGE_SERIALIZE | IFACE_MOD_CALLBACK,
	INTERFACE_USBVIRT_CB =
	    FOURCC_COMPACT('u', 's', 'b', 'v') | IFACE_EXCHANGE_PARALLEL | IFACE_MOD_CALLBACK,
	INTERFACE_ADB_CB =
	    FOURCC_COMPACT('a', 'd', 'b', ' ') | IFACE_EXCHANGE_SERIALIZE | IFACE_MOD_CALLBACK,
	INTERFACE_MOUSE_CB =
	    FOURCC_COMPACT('m', 'o', 'u', 's') | IFACE_EXCHANGE_SERIALIZE | IFACE_MOD_CALLBACK,
	INTERFACE_KBD_CB =
	    FOURCC_COMPACT('k', 'b', 'd', ' ') | IFACE_EXCHANGE_SERIALIZE | IFACE_MOD_CALLBACK,
	INTERFACE_VOL =
	    FOURCC_COMPACT('v', 'o', 'l', ' ') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_VBD =
	    FOURCC_COMPACT('v', 'b', 'd', ' ') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_IPC_TEST =
	    FOURCC_COMPACT('i', 'p', 'c', 't') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_PCI =
	    FOURCC_COMPACT('p', 'c', 'i', ' ') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_DDEV =
	    FOURCC_COMPACT('d', 'd', 'e', 'v') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_DISPCFG =
	    FOURCC_COMPACT('d', 'c', 'f', 'g') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_DISPCFG_CB =
	    FOURCC_COMPACT('d', 'c', 'f', 'g') | IFACE_EXCHANGE_SERIALIZE | IFACE_MOD_CALLBACK,
	INTERFACE_DISPLAY =
	    FOURCC_COMPACT('d', 's', 'p', 'l') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_DISPLAY_CB =
	    FOURCC_COMPACT('d', 's', 'p', 'l') | IFACE_EXCHANGE_SERIALIZE | IFACE_MOD_CALLBACK,
	INTERFACE_GC =
	    FOURCC_COMPACT('g', 'f', 'x', 'c') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_WNDMGT =
	    FOURCC_COMPACT('w', 'm', 'g', 't') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_WNDMGT_CB =
	    FOURCC_COMPACT('w', 'm', 'g', 't') | IFACE_EXCHANGE_SERIALIZE | IFACE_MOD_CALLBACK,
	INTERFACE_TBARCFG_NOTIFY =
	    FOURCC_COMPACT('t', 'b', 'c', 'f') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_SYSTEM =
	    FOURCC_COMPACT('s', 's', 't', 'm') | IFACE_EXCHANGE_SERIALIZE,
	INTERFACE_SYSTEM_CB =
	    FOURCC_COMPACT('s', 's', 't', 'm') | IFACE_EXCHANGE_SERIALIZE | IFACE_MOD_CALLBACK
} iface_t;

#endif

/** @}
 */
