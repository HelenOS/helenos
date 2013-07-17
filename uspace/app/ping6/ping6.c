/*
 * Copyright (c) 2013 Antonin Steinhauser
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

/** @addtogroup ping6
 * @{
 */
/** @file ICMPv6 echo utility.
 */

#include <async.h>
#include <stdbool.h>
#include <errno.h>
#include <fibril_synch.h>
#include <net/socket_codes.h>
#include <inet/dnsr.h>
#include <inet/addr.h>
#include <inet/inetping6.h>
#include <io/console.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define NAME "ping6"

/** Delay between subsequent ping requests in microseconds */
#define PING_DELAY (1000 * 1000)

/** Ping request timeout in microseconds */
#define PING_TIMEOUT (1000 * 1000)

static int ping_ev_recv(inetping6_sdu_t *);

static bool done = false;
static FIBRIL_CONDVAR_INITIALIZE(done_cv);
static FIBRIL_MUTEX_INITIALIZE(done_lock);

static inetping6_ev_ops_t ev_ops = {
	.recv = ping_ev_recv
};

static addr128_t src;
static addr128_t dest;

static bool ping_repeat = false;

static void print_syntax(void)
{
	printf("syntax: " NAME " [-r] <host>\n");
}

static void ping_signal_done(void)
{
	fibril_mutex_lock(&done_lock);
	done = true;
	fibril_mutex_unlock(&done_lock);
	fibril_condvar_broadcast(&done_cv);
}

static int ping_ev_recv(inetping6_sdu_t *sdu)
{
	inet_addr_t src_addr;
	inet_addr_set6(sdu->src, &src_addr);
	
	inet_addr_t dest_addr;
	inet_addr_set6(sdu->dest, &dest_addr);
	
	char *asrc;
	int rc = inet_addr_format(&src_addr, &asrc);
	if (rc != EOK)
		return ENOMEM;
	
	char *adest;
	rc = inet_addr_format(&dest_addr, &adest);
	if (rc != EOK) {
		free(asrc);
		return ENOMEM;
	}
	
	printf("Received ICMPv6 echo reply: from %s to %s, seq. no %u, "
	    "payload size %zu\n", asrc, adest, sdu->seq_no, sdu->size);
	
	if (!ping_repeat)
		ping_signal_done();
	
	free(asrc);
	free(adest);
	return EOK;
}

static int ping_send(uint16_t seq_no)
{
	inetping6_sdu_t sdu;
	int rc;

	addr128(src, sdu.src);
	addr128(dest, sdu.dest);
	sdu.seq_no = seq_no;
	sdu.data = (void *) "foo";
	sdu.size = 3;

	rc = inetping6_send(&sdu);
	if (rc != EOK) {
		printf(NAME ": Failed sending echo request (%d).\n", rc);
		return rc;
	}

	return EOK;
}

static int transmit_fibril(void *arg)
{
	uint16_t seq_no = 0;

	while (true) {
		fibril_mutex_lock(&done_lock);
		if (done) {
			fibril_mutex_unlock(&done_lock);
			return 0;
		}
		fibril_mutex_unlock(&done_lock);

		(void) ping_send(++seq_no);
		async_usleep(PING_DELAY);
	}

	return 0;
}

static int input_fibril(void *arg)
{
	console_ctrl_t *con;
	cons_event_t ev;

	con = console_init(stdin, stdout);
	printf("[Press Ctrl-Q to quit]\n");

	while (true) {
		if (!console_get_event(con, &ev))
			break;

		if (ev.type == CEV_KEY && ev.ev.key.type == KEY_PRESS &&
		    (ev.ev.key.mods & (KM_ALT | KM_SHIFT)) ==
		    0 && (ev.ev.key.mods & KM_CTRL) != 0) {
			/* Ctrl+key */
			if (ev.ev.key.key == KC_Q) {
				ping_signal_done();
				return 0;
			}
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	dnsr_hostinfo_t *hinfo = NULL;
	char *asrc = NULL;
	char *adest = NULL;
	char *sdest = NULL;
	int rc;
	int argi;

	rc = inetping6_init(&ev_ops);
	if (rc != EOK) {
		printf(NAME ": Failed connecting to internet ping service "
		    "(%d).\n", rc);
		goto error;
	}

	argi = 1;
	if (argi < argc && str_cmp(argv[argi], "-r") == 0) {
		ping_repeat = true;
		++argi;
	} else {
		ping_repeat = false;
	}

	if (argc - argi != 1) {
		print_syntax();
		goto error;
	}

	/* Parse destination address */
	inet_addr_t dest_addr;
	rc = inet_addr_parse(argv[argi], &dest_addr);
	if (rc != EOK) {
		/* Try interpreting as a host name */
		rc = dnsr_name2host(argv[argi], &hinfo);
		if (rc != EOK) {
			printf(NAME ": Error resolving host '%s'.\n", argv[argi]);
			goto error;
		}
		
		dest_addr = hinfo->addr;
	}
	
	uint16_t af = inet_addr_get(&dest_addr, NULL, &dest);
	if (af != AF_INET6) {
		printf(NAME ": Destination '%s' is not an IPv6 address.\n",
		    argv[argi]);
		goto error;
	}
	
	/* Determine source address */
	rc = inetping6_get_srcaddr(dest, src);
	if (rc != EOK) {
		printf(NAME ": Failed determining source address.\n");
		goto error;
	}
	
	inet_addr_t src_addr;
	inet_addr_set6(src, &src_addr);
	
	rc = inet_addr_format(&src_addr, &asrc);
	if (rc != EOK) {
		printf(NAME ": Out of memory.\n");
		goto error;
	}
	
	rc = inet_addr_format(&dest_addr, &adest);
	if (rc != EOK) {
		printf(NAME ": Out of memory.\n");
		goto error;
	}
	
	if (hinfo != NULL) {
		rc = asprintf(&sdest, "%s (%s)", hinfo->cname, adest);
		if (rc < 0) {
			printf(NAME ": Out of memory.\n");
			goto error;
		}
	} else {
		sdest = adest;
		adest = NULL;
	}

	printf("Sending ICMP echo request from %s to %s.\n",
	    asrc, sdest);

	fid_t fid;

	if (ping_repeat) {
		fid = fibril_create(transmit_fibril, NULL);
		if (fid == 0) {
			printf(NAME ": Failed creating transmit fibril.\n");
			goto error;
		}

		fibril_add_ready(fid);

		fid = fibril_create(input_fibril, NULL);
		if (fid == 0) {
			printf(NAME ": Failed creating input fibril.\n");
			goto error;
		}

		fibril_add_ready(fid);
	} else {
		ping_send(1);
	}

	fibril_mutex_lock(&done_lock);
	rc = EOK;
	while (!done && rc != ETIMEOUT) {
		rc = fibril_condvar_wait_timeout(&done_cv, &done_lock,
			ping_repeat ? 0 : PING_TIMEOUT);
	}
	fibril_mutex_unlock(&done_lock);

	if (rc == ETIMEOUT) {
		printf(NAME ": Echo request timed out.\n");
		goto error;
	}

	free(asrc);
	free(adest);
	free(sdest);
	dnsr_hostinfo_destroy(hinfo);
	return 0;
	
error:
	free(asrc);
	free(adest);
	free(sdest);
	dnsr_hostinfo_destroy(hinfo);
	return 1;
}

/** @}
 */
