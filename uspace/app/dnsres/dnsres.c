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
#include <inet/addr2.h>
#include <inet/dnsr.h>
#include <stdio.h>
#include <stdlib.h>

#define NAME "dnsres"

static void print_syntax(void)
{
	printf("syntax: " NAME " <host-name>\n");
}

int main(int argc, char *argv[])
{
	int rc;
	dnsr_hostinfo_t *hinfo;
	char *hname;
	char *saddr;

	if (argc != 2) {
		print_syntax();
		return 1;
	}

	hname = argv[1];

	rc = dnsr_name2host(hname, &hinfo);
	if (rc != EOK) {
		printf(NAME ": Error resolving '%s'.\n", argv[1]);
		return 1;
	}

	rc = inet2_addr_format(&hinfo->addr, &saddr);
	if (rc != EOK) {
		dnsr_hostinfo_destroy(hinfo);
		printf(NAME ": Out of memory.\n");
		return 1;
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
