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

#ifndef LIBIPC_SERVICES_H_
#define LIBIPC_SERVICES_H_

typedef enum {
	SERVICE_LOAD = 1,
	SERVICE_PCI,
	SERVICE_VIDEO,
	SERVICE_CONSOLE,
	SERVICE_VFS,
	SERVICE_DEVMAP,
	SERVICE_DEVMAN,
	SERVICE_FHC,
	SERVICE_OBIO,
	SERVICE_CLIPBOARD,
	SERVICE_NETWORKING,
	SERVICE_LO,
	SERVICE_DP8390,
	SERVICE_ETHERNET,
	SERVICE_NILDUMMY,
	SERVICE_IP,
	SERVICE_ARP,
	SERVICE_RARP,
	SERVICE_ICMP,
	SERVICE_UDP,
	SERVICE_TCP,
	SERVICE_SOCKET
} services_t;

/* Memory area to be received from NS */
#define SERVICE_MEM_REALTIME    1
#define SERVICE_MEM_KLOG        2

#endif

/** @}
 */
