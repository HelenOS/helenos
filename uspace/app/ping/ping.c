/*
 * Copyright (c) 2012 Jiri Svoboda
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

#include <errno.h>
#include <fibril_synch.h>
#include <inet/inetping.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#define NAME "ping"

/** Ping request timeout in microseconds */
#define PING_TIMEOUT (1000 * 1000)

static int ping_ev_recv(inetping_sdu_t *);
static FIBRIL_CONDVAR_INITIALIZE(done_cv);
static FIBRIL_MUTEX_INITIALIZE(done_lock);

static inetping_ev_ops_t ev_ops = {
	.recv = ping_ev_recv
};

static void print_syntax(void)
{
	printf("syntax: " NAME " <addr>\n");
}

static int addr_parse(const char *text, inet_addr_t *addr)
{
	unsigned long a[4];
	char *cp = (char *)text;
	int i;

	for (i = 0; i < 3; i++) {
		a[i] = strtoul(cp, &cp, 10);
		if (*cp != '.')
			return EINVAL;
		++cp;
	}

	a[3] = strtoul(cp, &cp, 10);
	if (*cp != '\0')
		return EINVAL;

	addr->ipv4 = 0;
	for (i = 0; i < 4; i++) {
		if (a[i] > 255)
			return EINVAL;
		addr->ipv4 = (addr->ipv4 << 8) | a[i];
	}

	return EOK;
}

static int addr_format(inet_addr_t *addr, char **bufp)
{
	int rc;

	rc = asprintf(bufp, "%d.%d.%d.%d", addr->ipv4 >> 24,
	    (addr->ipv4 >> 16) & 0xff, (addr->ipv4 >> 8) & 0xff,
	    addr->ipv4 & 0xff);

	if (rc < 0)
		return ENOMEM;

	return EOK;
}

static int ping_ev_recv(inetping_sdu_t *sdu)
{
	char *asrc, *adest;
	int rc;

	rc = addr_format(&sdu->src, &asrc);
	if (rc != EOK)
		return ENOMEM;

	rc = addr_format(&sdu->dest, &adest);
	if (rc != EOK) {
		free(asrc);
		return ENOMEM;
	}
	printf("Received ICMP echo reply: from %s to %s, seq. no %u, "
	    "payload size %zu\n", asrc, adest, sdu->seq_no, sdu->size);

	fibril_condvar_broadcast(&done_cv);

	free(asrc);
	free(adest);
	return EOK;
}

int main(int argc, char *argv[])
{
	int rc;
	inetping_sdu_t sdu;

	rc = inetping_init(&ev_ops);
	if (rc != EOK) {
		printf(NAME ": Failed connecting to internet ping service "
		    "(%d).\n", rc);
		return 1;
	}

	if (argc != 2) {
		print_syntax();
		return 1;
	}

	/* Parse destination address */
	rc = addr_parse(argv[1], &sdu.dest);
	if (rc != EOK) {
		printf(NAME ": Invalid address format.\n");
		print_syntax();
		return 1;
	}

	sdu.seq_no = 1;
	sdu.data = (void *) "foo";
	sdu.size = 3;

	/* Determine source address */
	rc = inetping_get_srcaddr(&sdu.dest, &sdu.src);
	if (rc != EOK) {
		printf(NAME ": Failed determining source address.\n");
		return 1;
	}

	rc = inetping_send(&sdu);
	if (rc != EOK) {
		printf(NAME ": Failed sending echo request (%d).\n", rc);
		return 1;
	}

	fibril_mutex_lock(&done_lock);
	rc = fibril_condvar_wait_timeout(&done_cv, &done_lock, PING_TIMEOUT);
	fibril_mutex_unlock(&done_lock);

	if (rc == ETIMEOUT) {
		printf(NAME ": Echo request timed out.\n");
		return 1;
	}

	return 0;
}

/** @}
 */
