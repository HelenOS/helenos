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

/** @addtogroup dnsres
 * @{
 */
/** @file DNS query utility.
 */

#include <errno.h>
#include <inet/addr.h>
#include <inet/dnsr.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>

#define NAME  "dnsres"

static void print_syntax(void)
{
	printf("Syntax: %s [-4|-6] <host-name>\n", NAME);
}

int main(int argc, char *argv[])
{
	if ((argc < 2) || (argc > 3)) {
		print_syntax();
		return 1;
	}

	uint16_t ver;
	char *hname;

	if (str_cmp(argv[1], "-4") == 0) {
		if (argc < 3) {
			print_syntax();
			return 1;
		}

		ver = ip_v4;
		hname = argv[2];
	} else if (str_cmp(argv[1], "-6") == 0) {
		if (argc < 3) {
			print_syntax();
			return 1;
		}

		ver = ip_v6;
		hname = argv[2];
	} else {
		ver = ip_any;
		hname = argv[1];
	}

	dnsr_hostinfo_t *hinfo;
	errno_t rc = dnsr_name2host(hname, &hinfo, ver);
	if (rc != EOK) {
		printf("%s: Error resolving '%s'.\n", NAME, hname);
		return rc;
	}

	char *saddr;
	rc = inet_addr_format(&hinfo->addr, &saddr);
	if (rc != EOK) {
		dnsr_hostinfo_destroy(hinfo);
		printf("%s: Error formatting address.\n", NAME);
		return rc;
	}

	printf("Host name: %s\n", hname);

	if (str_cmp(hname, hinfo->cname) != 0)
		printf("Canonical name: %s\n", hinfo->cname);

	printf("Address: %s\n", saddr);

	dnsr_hostinfo_destroy(hinfo);
	free(saddr);

	return 0;
}

/** @}
 */
