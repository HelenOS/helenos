/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
