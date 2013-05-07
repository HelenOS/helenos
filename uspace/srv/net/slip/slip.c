/*
 * Copyright (c) 2013 Jakub Jermar
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

/** @addtogroup slip
 * @{
 */
/**
 * @file
 * @brief IP over serial line IP link provider.
 */

#include <stdio.h>
#include <loc.h>
#include <inet/iplink_srv.h>
#include <io/log.h>
#include <errno.h>

#define NAME		"slip"
#define CAT_IPLINK	"iplink"

#define SLIP_MTU	1006	/* as per RFC 1055 */

static int slip_open(iplink_srv_t *);
static int slip_close(iplink_srv_t *);
static int slip_send(iplink_srv_t *, iplink_srv_sdu_t *);
static int slip_get_mtu(iplink_srv_t *, size_t *);
static int slip_addr_add(iplink_srv_t *, iplink_srv_addr_t *);
static int slip_addr_remove(iplink_srv_t *, iplink_srv_addr_t *);

static iplink_srv_t slip_iplink;

static iplink_ops_t slip_iplink_ops = {
	.open = slip_open,
	.close = slip_close,
	.send = slip_send,
	.get_mtu = slip_get_mtu,
	.addr_add = slip_addr_add,
	.addr_remove = slip_addr_remove
};

int slip_open(iplink_srv_t *srv)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "slip_open()");
	return EOK;
}

int slip_close(iplink_srv_t *srv)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "slip_close()");
	return EOK;
}

int slip_send(iplink_srv_t *srv, iplink_srv_sdu_t *sdu)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "slip_send()");
	return ENOTSUP;
}

int slip_get_mtu(iplink_srv_t *srv, size_t *mtu)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "slip_get_mtu()");
	*mtu = SLIP_MTU;
	return EOK;
}

int slip_addr_add(iplink_srv_t *srv, iplink_srv_addr_t *addr)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "slip_addr_add()");
	return EOK;
}

int slip_addr_remove(iplink_srv_t *srv, iplink_srv_addr_t *addr)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "slip_addr_remove()");
	return EOK;
}

static void usage(void)
{
	printf("Usage: " NAME " <service-name> <link-name>\n");
}

static void slip_client_conn(ipc_callid_t iid, ipc_call_t *icall, void *arg)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "slip_client_conn()");
	iplink_conn(iid, icall, &slip_iplink);
}

static int slip_init(const char *svcstr, const char *linkstr)
{
	service_id_t svcid;
	service_id_t linksid;
	category_id_t iplinkcid;
	async_sess_t *sess = NULL;
	int rc;

	iplink_srv_init(&slip_iplink);
	slip_iplink.ops = &slip_iplink_ops;

	async_set_client_connection(slip_client_conn);

	rc = loc_server_register(NAME);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Failed registering server.");
		return rc;
	}

	rc = loc_service_get_id(svcstr, &svcid, 0);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Failed getting ID for service %s", svcstr);
		return rc;
	}

	rc = loc_category_get_id(CAT_IPLINK, &iplinkcid, 0);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Failed to get category ID for %s",
		    CAT_IPLINK);
		return rc;
	}

	sess = loc_service_connect(EXCHANGE_SERIALIZE, svcid, 0);
	if (!sess) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Failed to connect to service %s (ID=%d)",
		    svcstr, (int) svcid);
		return rc;
	}

	rc = loc_service_register(linkstr, &linksid);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		   "Failed to register service %s",
		   linkstr);
		goto fail;
	}

	rc = loc_service_add_to_cat(linksid, iplinkcid);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Filed to add service %d (%s) to category %d (%s).",
		    (int) linksid, linkstr, (int) iplinkcid, CAT_IPLINK);
		goto fail;
	}

	return EOK;

fail:
	if (sess)
		async_hangup(sess);

	/*
 	 * We assume that our registration at the location service will be
 	 * cleaned up automatically as the service (i.e. this task) terminates.
 	 */

	return rc;
}

int main(int argc, char *argv[])
{
	int rc;

	printf(NAME ": IP over serial line service\n");

	if (argc != 3) {
		usage();
		return EINVAL;
	}

	rc = log_init(NAME);
	if (rc != EOK) {
		printf(NAME ": failed to initialize log\n");
		return rc;
	}

	rc = slip_init(argv[1], argv[2]);
	if (rc != EOK)
		return 1;

	printf(NAME ": Accepting connections.\n");
	task_retval(0);
	async_manager();

	/* not reached */
	return 0;
}

/** @}
 */
