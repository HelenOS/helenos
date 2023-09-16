/*
 * Copyright (c) 2023 Jiri Svoboda
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
#include <stdint.h>
#include <stdlib.h>
#include <loc.h>
#include <inet/addr.h>
#include <inet/eth_addr.h>
#include <inet/iplink_srv.h>
#include <io/chardev.h>
#include <io/log.h>
#include <errno.h>
#include <str_error.h>
#include <task.h>

#define NAME		"slip"
#define CAT_IPLINK	"iplink"

#define SLIP_MTU	1006	/* as per RFC 1055 */

#define SLIP_END	0300
#define SLIP_ESC	0333
#define SLIP_ESC_END	0334
#define SLIP_ESC_ESC	0335

static errno_t slip_open(iplink_srv_t *);
static errno_t slip_close(iplink_srv_t *);
static errno_t slip_send(iplink_srv_t *, iplink_sdu_t *);
static errno_t slip_send6(iplink_srv_t *, iplink_sdu6_t *);
static errno_t slip_get_mtu(iplink_srv_t *, size_t *);
static errno_t slip_get_mac48(iplink_srv_t *, eth_addr_t *);
static errno_t slip_addr_add(iplink_srv_t *, inet_addr_t *);
static errno_t slip_addr_remove(iplink_srv_t *, inet_addr_t *);

static iplink_srv_t slip_iplink;

static iplink_ops_t slip_iplink_ops = {
	.open = slip_open,
	.close = slip_close,
	.send = slip_send,
	.send6 = slip_send6,
	.get_mtu = slip_get_mtu,
	.get_mac48 = slip_get_mac48,
	.addr_add = slip_addr_add,
	.addr_remove = slip_addr_remove
};

static uint8_t slip_send_buf[SLIP_MTU + 2];
static size_t slip_send_pending;

static uint8_t slip_recv_buf[SLIP_MTU + 2];
static size_t slip_recv_pending;
static size_t slip_recv_read;

errno_t slip_open(iplink_srv_t *srv)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "slip_open()");
	return EOK;
}

errno_t slip_close(iplink_srv_t *srv)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "slip_close()");
	return EOK;
}

static void write_flush(chardev_t *chardev)
{
	size_t written = 0;

	while (slip_send_pending > 0) {
		errno_t rc;
		size_t nwr;

		rc = chardev_write(chardev, &slip_send_buf[written],
		    slip_send_pending, &nwr);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR,
			    "chardev_write() -> %s", str_error_name(rc));
			slip_send_pending = 0;
			break;
		}
		written += nwr;
		slip_send_pending -= nwr;
	}
}

static void write_buffered(chardev_t *chardev, uint8_t ch)
{
	if (slip_send_pending == sizeof(slip_send_buf))
		write_flush(chardev);
	slip_send_buf[slip_send_pending++] = ch;
}

errno_t slip_send(iplink_srv_t *srv, iplink_sdu_t *sdu)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "slip_send()");

	chardev_t *chardev = (chardev_t *) srv->arg;
	uint8_t *data = sdu->data;

	/*
	 * Strictly speaking, this is not prescribed by the RFC, but the RFC
	 * suggests to start with sending a SLIP_END byte as a synchronization
	 * measure for dealing with previous possible noise on the line.
	 */
	write_buffered(chardev, SLIP_END);

	for (size_t i = 0; i < sdu->size; i++) {
		switch (data[i]) {
		case SLIP_END:
			write_buffered(chardev, SLIP_ESC);
			write_buffered(chardev, SLIP_ESC_END);
			break;
		case SLIP_ESC:
			write_buffered(chardev, SLIP_ESC);
			write_buffered(chardev, SLIP_ESC_ESC);
			break;
		default:
			write_buffered(chardev, data[i]);
			break;
		}
	}

	write_buffered(chardev, SLIP_END);
	write_flush(chardev);

	return EOK;
}

errno_t slip_send6(iplink_srv_t *srv, iplink_sdu6_t *sdu)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "slip_send6()");

	return ENOTSUP;
}

errno_t slip_get_mtu(iplink_srv_t *srv, size_t *mtu)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "slip_get_mtu()");
	*mtu = SLIP_MTU;
	return EOK;
}

errno_t slip_get_mac48(iplink_srv_t *src, eth_addr_t *mac)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "slip_get_mac48()");
	return ENOTSUP;
}

errno_t slip_addr_add(iplink_srv_t *srv, inet_addr_t *addr)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "slip_addr_add()");
	return EOK;
}

errno_t slip_addr_remove(iplink_srv_t *srv, inet_addr_t *addr)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "slip_addr_remove()");
	return EOK;
}

static void usage(void)
{
	printf("Usage: " NAME " <service-name> <link-name>\n");
}

static void slip_client_conn(ipc_call_t *icall, void *arg)
{
	log_msg(LOG_DEFAULT, LVL_DEBUG, "slip_client_conn()");
	iplink_conn(icall, &slip_iplink);
}

static uint8_t read_buffered(chardev_t *chardev)
{
	while (slip_recv_pending == 0) {
		errno_t rc;
		size_t nread;

		rc = chardev_read(chardev, slip_recv_buf,
		    sizeof(slip_recv_buf), &nread, chardev_f_none);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR,
			    "char_dev_read() -> %s", str_error_name(rc));
		}

		if (nread == 0)
			return SLIP_END;

		slip_recv_pending = nread;
		slip_recv_read = 0;
	}
	slip_recv_pending--;
	return slip_recv_buf[slip_recv_read++];
}

static errno_t slip_recv_fibril(void *arg)
{
	chardev_t *chardev = (chardev_t *) arg;
	static uint8_t recv_final[SLIP_MTU];
	iplink_recv_sdu_t sdu;
	uint8_t ch;
	errno_t rc;

	sdu.data = recv_final;

	while (true) {
		sdu.size = 0;
		while (sdu.size < sizeof(recv_final)) {
			ch = read_buffered(chardev);
			switch (ch) {
			case SLIP_END:
				if (sdu.size == 0) {
					/*
					 * Discard the empty SLIP datagram.
					 */
					break;
				}
				goto pass;

			case SLIP_ESC:
				ch = read_buffered(chardev);
				if (ch == SLIP_ESC_END) {
					recv_final[sdu.size++] = SLIP_END;
					break;
				} else if (ch == SLIP_ESC_ESC) {
					recv_final[sdu.size++] = SLIP_ESC;
					break;
				}

				/*
				 * The RFC suggests to simply insert the wrongly
				 * escaped character into the packet so we fall
				 * through.
				 */
				/* Fallthrough */

			default:
				recv_final[sdu.size++] = ch;
				break;
			}

		}

		/*
		 * We have reached the limit of our MTU. Regardless of whether
		 * the datagram is properly ended with SLIP_END, pass it along.
		 * If the next character is really SLIP_END, nothing
		 * catastrophic happens. The algorithm will just see an
		 * artificially empty SLIP datagram and life will go on.
		 */

	pass:
		rc = iplink_ev_recv(&slip_iplink, &sdu, ip_v4);
		if (rc != EOK) {
			log_msg(LOG_DEFAULT, LVL_ERROR,
			    "iplink_ev_recv() -> %s", str_error_name(rc));
		}
	}

	return 0;
}

static errno_t slip_init(const char *svcstr, const char *linkstr)
{
	service_id_t svcid;
	service_id_t linksid;
	category_id_t iplinkcid;
	async_sess_t *sess_in = NULL;
	async_sess_t *sess_out = NULL;
	chardev_t *chardev_in = NULL;
	chardev_t *chardev_out = NULL;
	fid_t fid;
	loc_srv_t *srv;
	errno_t rc;

	iplink_srv_init(&slip_iplink);
	slip_iplink.ops = &slip_iplink_ops;

	async_set_fallback_port_handler(slip_client_conn, NULL);

	rc = loc_server_register(NAME, &srv);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Failed registering server.");
		return rc;
	}

	rc = loc_service_get_id(svcstr, &svcid, 0);
	if (rc != EOK) {
		loc_server_unregister(srv);
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Failed getting ID for service %s", svcstr);
		return rc;
	}

	rc = loc_category_get_id(CAT_IPLINK, &iplinkcid, 0);
	if (rc != EOK) {
		loc_server_unregister(srv);
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Failed to get category ID for %s",
		    CAT_IPLINK);
		return rc;
	}

	/*
	 * Create two sessions to allow to both read and write from the
	 * char_dev at the same time.
	 */
	sess_out = loc_service_connect(svcid, INTERFACE_DDF, 0);
	if (!sess_out) {
		loc_server_unregister(srv);
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Failed to connect to service %s (ID=%d)",
		    svcstr, (int) svcid);
		return ENOENT;
	}

	rc = chardev_open(sess_out, &chardev_out);
	if (rc != EOK) {
		loc_server_unregister(srv);
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Failed opening character device.");
		return ENOENT;
	}

	slip_iplink.arg = chardev_out;

	sess_in = loc_service_connect(svcid, INTERFACE_DDF, 0);
	if (!sess_in) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Failed to connect to service %s (ID=%d)",
		    svcstr, (int) svcid);
		rc = ENOENT;
		goto fail;
	}

	rc = chardev_open(sess_in, &chardev_in);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Failed opening character device.");
		return ENOENT;
	}

	rc = loc_service_register(srv, linkstr, &linksid);
	if (rc != EOK) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Failed to register service %s",
		    linkstr);
		goto fail;
	}

	rc = loc_service_add_to_cat(srv, linksid, iplinkcid);
	if (rc != EOK) {
		loc_service_unregister(srv, linksid);
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Failed to add service %d (%s) to category %d (%s).",
		    (int) linksid, linkstr, (int) iplinkcid, CAT_IPLINK);
		goto fail;
	}

	fid = fibril_create(slip_recv_fibril, chardev_in);
	if (!fid) {
		log_msg(LOG_DEFAULT, LVL_ERROR,
		    "Failed to create receive fibril.");
		rc = ENOENT;
		goto fail;
	}
	fibril_add_ready(fid);

	return EOK;

fail:
	loc_server_unregister(srv);
	chardev_close(chardev_out);
	if (sess_out)
		async_hangup(sess_out);
	chardev_close(chardev_in);
	if (sess_in)
		async_hangup(sess_in);

	/*
	 * We assume that our registration at the location service will be
	 * cleaned up automatically as the service (i.e. this task) terminates.
	 */

	return rc;
}

int main(int argc, char *argv[])
{
	errno_t rc;

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
