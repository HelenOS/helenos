/*
 * Copyright (c) 2023 Nataliia Korop
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

/** @addtogroup libpcap
 * @{
 */
/** @file pcap interface
 */

#ifndef PCAP_IFACE_H_
#define PCAP_IFACE_H_

#include <errno.h>
#include "pcap.h"

typedef struct pcap_iface {
	bool to_dump;
	errno_t (*init)(const char *);
	void (*add_packet)(const void *data, size_t size);
	void (*fini)(void);
} pcap_iface_t;

extern void pcap_close_file(void);
extern errno_t pcap_iface_init(pcap_iface_t *);
//init to file
//init to serial
//add packet, dostane strukturu, data, velikost ... to to this pcap_iface_t
// v ramci init jeste linktype prg
//set snaplen taky lze pridavat prg
//create kam posila 
// init
extern errno_t pcap_init(const char *);
extern void pcap_add_packet(const void *data, size_t size);

#endif
/** @}
 */
