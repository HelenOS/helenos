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
 * @file
 * @brief Server side of the pcapctl
 */

#include <async.h>
#include <errno.h>
#include <stdlib.h>
#include <fibril_synch.h>
#include <str.h>
#include <io/log.h>

#include "pcap_dumper.h"
#include "pcapdump_srv.h"
#include "pcapdump_ipc.h"

/** Start dumping.
 * 	@param icall 	IPC call with request to start.
 *  @param dumper 	Dumping interface of the driver.
 */
static void pcapdump_start_srv(ipc_call_t *icall, pcap_dumper_t *dumper)
{
	char *data;
	size_t size;
	int ops_index = (int)ipc_get_arg1(icall);
	errno_t rc = async_data_write_accept((void **) &data, true, 0, 0, 0, &size);
	if (rc != EOK) {
		async_answer_0(icall, rc);
		return;
	}

	assert(str_length(data) == size && "Data were damaged during transmission.\n");

	// Deadlock solution when trying to start dump while dumping (to the same device)
	if (dumper->to_dump) {
		free(data);
		log_msg(LOG_DEFAULT, LVL_ERROR, "Trying to start dumping while dumping.\n");
		async_answer_0(icall, EBUSY);
		return;
	}

	rc = pcap_dumper_set_ops(dumper, ops_index);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Setting ops for dumper was not successful.\n");
		free(data);
		async_answer_0(icall, EOK);
		return;
	}

	rc = pcap_dumper_start(dumper, (const char *)data);
	free(data);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_DEBUG, "Starting the dumping was not successful.\n");
	}
	async_answer_0(icall, EOK);
}

/** Stop dumping
 * 	@param icall 	IPC call with request to stop.
 *  @param dumper 	Dumping interface of the driver.
 */
static void pcapdump_stop_srv(ipc_call_t *icall, pcap_dumper_t *dumper)
{
	pcap_dumper_stop(dumper);
	async_answer_0(icall, EOK);
}

/** Get number of accessibke writer operations.
 * 	@param icall 	IPC call with request to get number of accessible writer operations.
 */
static void pcapdump_get_ops_num_srv(ipc_call_t *icall)
{
	size_t count = pcap_dumper_get_ops_number();

	log_msg(LOG_DEFAULT, LVL_NOTE, "Getting number of ops.\n");

	async_answer_1(icall, EOK, count);
}

/** Callback connection function. Accepts requests and processes them.
 * 	@param icall 	IPC call with request.
 *  @param arg	 	Dumping interface of the driver.
 */
void pcapdump_conn(ipc_call_t *icall, void *arg)
{
	pcap_dumper_t *dumper = (pcap_dumper_t *)arg;

	assert((dumper != NULL) && "pcapdump requires pcap dumper\n");

	/* Accept connection */
	async_accept_0(icall);

	while (true) {
		ipc_call_t call;
		async_get_call(&call);
		sysarg_t method = ipc_get_imethod(&call);
		if (!method) {
			/* The other side has hung up */
			async_answer_0(&call, EOK);
			break;
		}
		switch (method) {
		case PCAP_CONTROL_SET_START:
			pcapdump_start_srv(&call, dumper);
			break;
		case PCAP_CONTROL_SET_STOP:
			pcapdump_stop_srv(&call, dumper);
			break;
		case PCAP_CONTROL_GET_OPS_NUM:
			pcapdump_get_ops_num_srv(&call);
			break;
		default:
			async_answer_0(&call, EINVAL);
			break;
		}
	}
}

/** @}
 */
