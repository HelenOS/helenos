/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup libcipc
 * @{
 */
/**
 * @file  services.h
 * @brief List of all known services and their codes.
 */

#ifndef LIBC_SERVICES_H_
#define LIBC_SERVICES_H_

#include <fourcc.h>

typedef enum {
	SERVICE_NONE       = 0,
	SERVICE_LOAD       = FOURCC('l', 'o', 'a', 'd'),
	SERVICE_VFS        = FOURCC('v', 'f', 's', ' '),
	SERVICE_LOC        = FOURCC('l', 'o', 'c', ' '),
	SERVICE_LOGGER     = FOURCC('l', 'o', 'g', 'g'),
	SERVICE_DEVMAN     = FOURCC('d', 'e', 'v', 'n'),
	SERVICE_IRC        = FOURCC('i', 'r', 'c', ' '),
	SERVICE_CLIPBOARD  = FOURCC('c', 'l', 'i', 'p'),
	SERVICE_UDP        = FOURCC('u', 'd', 'p', ' '),
	SERVICE_TCP        = FOURCC('t', 'c', 'p', ' ')
} services_t;

#define SERVICE_NAME_CORECFG	"corecfg"
#define SERVICE_NAME_DHCP       "net/dhcp"
#define SERVICE_NAME_DNSR       "net/dnsr"
#define SERVICE_NAME_INET       "net/inet"
#define SERVICE_NAME_INETCFG    "net/inetcfg"
#define SERVICE_NAME_INETPING   "net/inetping"
#define SERVICE_NAME_INETPING6  "net/inetping6"
#define SERVICE_NAME_NETCONF    "net/netconf"

#endif

/** @}
 */
