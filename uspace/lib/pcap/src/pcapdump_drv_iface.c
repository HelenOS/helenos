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

/**
 * @addtogroup libpcap
 * @{
 */
/**
 * @file Functions that are called inside driver that can dump packets/
 *
 */

#include <async.h>
#include <errno.h>
#include <stdlib.h>
#include <fibril_synch.h>
#include <str.h>
#include <io/log.h>

#include "pcapdump_srv.h"
#include "pcapdump_drv_iface.h"

#define NAME "pcap"

/** Initialize interface for dumping packets
 *
 * @param dumper Device dumping interface
 *
 */
static errno_t pcapdump_drv_dumper_init(pcap_dumper_t *dumper)
{
	fibril_mutex_initialize(&dumper->mutex);
	dumper->to_dump = false;
	dumper->writer.ops = NULL;

	errno_t rc = log_init(NAME);
	if (rc != EOK) {
		printf("%s : Failed to initialize log.\n", NAME);
		return 1;
	}
	return EOK;
}

errno_t pcapdump_init(pcap_dumper_t *dumper)
{
	port_id_t port;
	errno_t rc;

	rc = pcapdump_drv_dumper_init(dumper);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Failed initializing pcap dumper: %s", str_error(rc));
		return rc;
	}

	rc = async_create_port(INTERFACE_PCAP_CONTROL, pcapdump_conn, dumper, &port);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Failed creating port for pcap dumper: %s", str_error(rc));
		return rc;
	}
	return EOK;
}

/** Dumping function for driver
 *
 * Called every time, the packet is sent/recieved by the device
 *
 * @param dumper Dumping interface
 * @param data The packet
 * @param size Size of the packet
 *
 */
void pcapdump_packet(pcap_dumper_t *dumper, const void *data, size_t size)
{
	if (dumper == NULL) {
		return;
	}
	pcap_dumper_add_packet(dumper, data, size);
}