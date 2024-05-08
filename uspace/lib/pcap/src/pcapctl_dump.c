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
/** @file
 * @brief Client side of the pcapctl. Functions are called from the app pcapctl
 */

#include <errno.h>
#include <async.h>
#include <str.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "pcapctl_dump.h"
#include "pcapdump_iface.h"

/** Finish an async exchange on the pcapctl session
 *
 * @param exch  Exchange to be finished
 */
static void pcapctl_dump_exchange_end(async_exch_t *exch)
{
	async_exchange_end(exch);
}

static errno_t pcapctl_cat_get_svc(int *index, service_id_t *svc)
{
	errno_t rc;
	category_id_t pcap_cat;
	size_t count;
	service_id_t *pcap_svcs = NULL;

	rc = loc_category_get_id("pcap", &pcap_cat, 0);
	if (rc != EOK) {
		printf("Error resolving category 'pcap'.\n");
		return rc;
	}

	rc = loc_category_get_svcs(pcap_cat, &pcap_svcs, &count);
	if (rc != EOK) {
		printf("Error resolving list of pcap services.\n");
		free(pcap_svcs);
		return rc;
	}
	if (*index < (int)count) {
		*svc =  pcap_svcs[*index];
		free(pcap_svcs);
		return EOK;
	}

	return ENOENT;
}

errno_t pcapctl_is_valid_device(int *index)
{
	errno_t rc;
	category_id_t pcap_cat;
	size_t count;
	service_id_t *pcap_svcs = NULL;

	rc = loc_category_get_id("pcap", &pcap_cat, 0);
	if (rc != EOK) {
		printf("Error resolving category pcap.\n");
		return rc;
	}

	rc = loc_category_get_svcs(pcap_cat, &pcap_svcs, &count);
	if (rc != EOK) {
		printf("Error resolving list of pcap services.\n");
		free(pcap_svcs);
		return rc;
	}
	if (*index + 1 > (int)count || *index < 0) {
		return EINVAL;
	}
	return EOK;
}

/**
 *
 */
errno_t pcapctl_list(void)
{
	errno_t rc;
	category_id_t pcap_cat;
	size_t count;
	service_id_t *pcap_svcs = NULL;

	rc = loc_category_get_id("pcap", &pcap_cat, 0);
	if (rc != EOK) {
		printf("Error resolving category pcap.\n");
		return rc;
	}

	rc = loc_category_get_svcs(pcap_cat, &pcap_svcs, &count);
	if (rc != EOK) {
		printf("Error resolving list of pcap services.\n");
		free(pcap_svcs);
		return rc;
	}

	fprintf(stdout, "Devices:\n");
	for (unsigned i = 0; i < count; ++i) {
		char *name = NULL;
		loc_service_get_name(pcap_svcs[i], &name);
		fprintf(stdout, "%d. %s\n", i, name);
	}
	free(pcap_svcs);
	return EOK;
}

/**
 *
 */
errno_t pcapctl_dump_open(int *index, pcapctl_sess_t **rsess)
{
	errno_t rc;
	service_id_t svc;
	pcapctl_sess_t *sess = calloc(1, sizeof(pcapctl_sess_t));
	if (sess == NULL)
		return ENOMEM;

	printf("number: %d\n", *index);
	if (*index == -1) {

		rc = loc_service_get_id("net/eth1", &svc, 0);
		if (rc != EOK)
		{
			fprintf(stderr, "Error getting service id.\n");
			return ENOENT;
		}
	}
	else {
		rc  = pcapctl_cat_get_svc(index, &svc);
		if (rc != EOK) {
			printf("Error finding the device with index: %d\n", *index);
			goto error;
		}
	}


	async_sess_t *new_session = loc_service_connect(svc, INTERFACE_PCAP_CONTROL, 0);
	if (new_session == NULL) {
		fprintf(stderr, "Error connecting to service.\n");
		rc =  EREFUSED;
		goto error;
	}

	sess->sess = new_session;
	*rsess = sess;
	return EOK;
error:
	pcapctl_dump_close(sess);
	return rc;
}

/**
 *
 */
errno_t pcapctl_dump_close(pcapctl_sess_t *sess)
{
	free(sess);
	return EOK;
}

/** Starting a new session for pcapctl
 *
 * @param name Name of the file to dump packets to
 * @param sess session to start
 * @return EOK on success or an error code
 */
errno_t pcapctl_dump_start(const char *name, pcapctl_sess_t *sess)
{
	errno_t rc;
	async_exch_t *exch = async_exchange_begin(sess->sess);

	size_t size = str_size(name);
	aid_t req = async_send_0(exch, PCAP_CONTROL_SET_START, NULL);

	rc = async_data_write_start(exch, (void *) name, size);

	pcapctl_dump_exchange_end(exch);

	if (rc != EOK) {
		async_forget(req);
		return rc;
	}

	errno_t retval;
	async_wait_for(req, &retval);
	return retval;
}

/** Finish current session for pcapctl
 *
 * @param sess Session to finish
 * @return EOK on success or an error code
 */
errno_t pcapctl_dump_stop(pcapctl_sess_t *sess)
{
	errno_t rc;
	async_exch_t *exch = async_exchange_begin(sess->sess);
	rc = async_req_0_0(exch, PCAP_CONTROL_SET_STOP);

	pcapctl_dump_exchange_end(exch);
	return rc;
}

/** @}
 */
