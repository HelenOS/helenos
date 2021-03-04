/*
 * Copyright (c) 2013 Jiri Svoboda
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

/** @addtogroup ping
 * @{
 */
/** @file ICMP echo utility.
 */

#include <async.h>
#include <errno.h>
#include <fibril_synch.h>
#include <inet/addr.h>
#include <inet/host.h>
#include <inet/inetping.h>
#include <io/console.h>
#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <str.h>
#include <str_error.h>

#define NAME "ping"

/** Delay between subsequent ping requests in microseconds */
#define PING_DELAY (1000 * 1000)

/** Ping request timeout in microseconds */
#define PING_TIMEOUT (1000 * 1000)

typedef enum {
	RECEIVED_NONE,
	RECEIVED_SUCCESS,
	RECEIVED_INTERRUPT
} received_t;

static received_t received;
static FIBRIL_CONDVAR_INITIALIZE(received_cv);
static FIBRIL_MUTEX_INITIALIZE(received_lock);

static bool quit = false;
static FIBRIL_CONDVAR_INITIALIZE(quit_cv);
static FIBRIL_MUTEX_INITIALIZE(quit_lock);

static errno_t ping_ev_recv(inetping_sdu_t *);

static inetping_ev_ops_t ev_ops = {
	.recv = ping_ev_recv
};

static inet_addr_t src_addr;
static inet_addr_t dest_addr;

static bool repeat_forever = false;
static size_t repeat_count = 1;

static const char *short_options = "46rn:";

static void print_syntax(void)
{
	printf("Syntax: %s [<options>] <host>\n", NAME);
	printf("\t-n <count> Repeat the specified number of times\n");
	printf("\t-r         Repeat forever\n");
	printf("\t-4|-6      Use IPv4 or IPv6 destination host address\n");
}

static void ping_signal_received(received_t value)
{
	fibril_mutex_lock(&received_lock);
	received = value;
	fibril_mutex_unlock(&received_lock);
	fibril_condvar_broadcast(&received_cv);
}

static void ping_signal_quit(void)
{
	fibril_mutex_lock(&quit_lock);
	quit = true;
	fibril_mutex_unlock(&quit_lock);
	fibril_condvar_broadcast(&quit_cv);
}

static errno_t ping_ev_recv(inetping_sdu_t *sdu)
{
	char *asrc;
	errno_t rc = inet_addr_format(&src_addr, &asrc);
	if (rc != EOK)
		return ENOMEM;

	char *adest;
	rc = inet_addr_format(&dest_addr, &adest);
	if (rc != EOK) {
		free(asrc);
		return ENOMEM;
	}

	printf("Received ICMP echo reply: from %s to %s, seq. no %u, "
	    "payload size %zu\n", asrc, adest, sdu->seq_no, sdu->size);

	ping_signal_received(RECEIVED_SUCCESS);

	free(asrc);
	free(adest);
	return EOK;
}

static errno_t ping_send(uint16_t seq_no)
{
	inetping_sdu_t sdu;

	sdu.src = src_addr;
	sdu.dest = dest_addr;
	sdu.seq_no = seq_no;
	sdu.data = (void *) "foo";
	sdu.size = 3;

	errno_t rc = inetping_send(&sdu);
	if (rc != EOK)
		printf("Failed sending echo request: %s: %s.\n",
		    str_error_name(rc), str_error(rc));

	return rc;
}

static errno_t transmit_fibril(void *arg)
{
	uint16_t seq_no = 0;

	while ((repeat_count--) || (repeat_forever)) {
		fibril_mutex_lock(&received_lock);
		received = RECEIVED_NONE;
		fibril_mutex_unlock(&received_lock);

		(void) ping_send(++seq_no);

		fibril_mutex_lock(&received_lock);
		errno_t rc = fibril_condvar_wait_timeout(&received_cv, &received_lock,
		    PING_TIMEOUT);
		received_t recv = received;
		fibril_mutex_unlock(&received_lock);

		if ((rc == ETIMEOUT) || (recv == RECEIVED_NONE))
			printf("Echo request timed out (seq. no %u)\n", seq_no);

		if (recv == RECEIVED_INTERRUPT)
			break;

		if ((repeat_count > 0) || (repeat_forever)) {
			fibril_mutex_lock(&received_lock);
			rc = fibril_condvar_wait_timeout(&received_cv, &received_lock,
			    PING_DELAY);
			recv = received;
			fibril_mutex_unlock(&received_lock);

			if (recv == RECEIVED_INTERRUPT)
				break;
		}
	}

	ping_signal_quit();
	return 0;
}

static errno_t input_fibril(void *arg)
{
	console_ctrl_t *con = console_init(stdin, stdout);

	while (true) {
		cons_event_t ev;
		errno_t rc;
		rc = console_get_event(con, &ev);
		if (rc != EOK) {
			ping_signal_received(RECEIVED_INTERRUPT);
			break;
		}

		if ((ev.type == CEV_KEY) && (ev.ev.key.type == KEY_PRESS) &&
		    ((ev.ev.key.mods & (KM_ALT | KM_SHIFT)) == 0) &&
		    ((ev.ev.key.mods & KM_CTRL) != 0)) {
			/* Ctrl+key */
			if (ev.ev.key.key == KC_Q) {
				ping_signal_received(RECEIVED_INTERRUPT);
				break;
			}
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	char *asrc = NULL;
	char *adest = NULL;
	char *sdest = NULL;
	char *host;
	const char *errmsg;
	ip_ver_t ip_ver = ip_any;

	errno_t rc = inetping_init(&ev_ops);
	if (rc != EOK) {
		printf("Failed connecting to internet ping service: "
		    "%s: %s.\n", str_error_name(rc), str_error(rc));
		goto error;
	}

	int c;
	while ((c = getopt(argc, argv, short_options)) != -1) {
		switch (c) {
		case 'r':
			repeat_forever = true;
			break;
		case 'n':
			rc = str_size_t(optarg, NULL, 10, true, &repeat_count);
			if (rc != EOK) {
				printf("Invalid repeat count.\n");
				print_syntax();
				goto error;
			}
			break;
		case '4':
			ip_ver = ip_v4;
			break;
		case '6':
			ip_ver = ip_v6;
			break;
		default:
			printf("Unknown option passed.\n");
			print_syntax();
			goto error;
		}
	}

	if (optind >= argc) {
		printf("IP address or host name not supplied.\n");
		print_syntax();
		goto error;
	}

	host = argv[optind];

	/* Look up host */
	rc = inet_host_plookup_one(host, ip_ver, &dest_addr, NULL, &errmsg);
	if (rc != EOK) {
		printf("Error resolving host '%s' (%s).\n", host, errmsg);
		goto error;
	}

	/* Determine source address */
	rc = inetping_get_srcaddr(&dest_addr, &src_addr);
	if (rc != EOK) {
		printf("Failed determining source address.\n");
		goto error;
	}

	rc = inet_addr_format(&src_addr, &asrc);
	if (rc != EOK) {
		printf("Out of memory.\n");
		goto error;
	}

	rc = inet_addr_format(&dest_addr, &adest);
	if (rc != EOK) {
		printf("Out of memory.\n");
		goto error;
	}

	if (asprintf(&sdest, "%s (%s)", host, adest) < 0) {
		printf("Out of memory.\n");
		goto error;
	}

	printf("Sending ICMP echo request from %s to %s (Ctrl+Q to quit)\n",
	    asrc, sdest);

	fid_t fid = fibril_create(transmit_fibril, NULL);
	if (fid == 0) {
		printf("Failed creating transmit fibril.\n");
		goto error;
	}

	fibril_add_ready(fid);

	fid = fibril_create(input_fibril, NULL);
	if (fid == 0) {
		printf("Failed creating input fibril.\n");
		goto error;
	}

	fibril_add_ready(fid);

	fibril_mutex_lock(&quit_lock);
	while (!quit)
		fibril_condvar_wait(&quit_cv, &quit_lock);
	fibril_mutex_unlock(&quit_lock);

	free(asrc);
	free(adest);
	free(sdest);
	return 0;

error:
	free(asrc);
	free(adest);
	free(sdest);
	return 1;
}

/** @}
 */
