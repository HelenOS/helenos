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

/** @addtogroup dnsres
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "dns_msg.h"
#include "dns_std.h"
#include "query.h"

#define NAME  "dnsres"

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

int main(int argc, char *argv[])
{
	dns_host_info_t *hinfo;
	char *astr;
	int rc;

	printf("%s: DNS Resolution Service\n", NAME);
	rc = dns_name2host(argc < 2 ? "helenos.org" : argv[1], &hinfo);
	printf("dns_name2host() -> rc = %d\n", rc);

	if (rc == EOK) {
		rc = addr_format(&hinfo->addr, &astr);
		if (rc != EOK) {
			dns_hostinfo_destroy(hinfo);
			printf("Out of memory\n");
			return ENOMEM;
		}

		printf("hostname: %s\n", hinfo->name);
		printf("IPv4 address: %s\n", astr);
		free(astr);
		dns_hostinfo_destroy(hinfo);
	}

	return 0;
}

/** @}
 */
