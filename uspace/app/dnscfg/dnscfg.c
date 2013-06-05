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

/** @addtogroup dnscfg
 * @{
 */
/** @file DNS configuration utility.
 *
 * Controls the DNS resolution server (@c dnsrsrv).
 */

#include <errno.h>
#include <inet/addr.h>
#include <inet/dnsr.h>
#include <ipc/services.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>
#include <sys/types.h>

#define NAME "dnscfg"

static void print_syntax(void)
{
	printf("syntax:\n");
	printf("\t" NAME " set-ns <server-addr>\n");
	printf("\t" NAME " unset-ns\n");
}

static int dnscfg_set_ns(int argc, char *argv[])
{
	char *srv_addr;
	inet_addr_t addr;
	int rc;

	if (argc < 1) {
		printf(NAME ": Missing arguments.\n");
		print_syntax();
		return EINVAL;
	}

	if (argc > 1) {
		printf(NAME ": Too many arguments.\n");
		print_syntax();
		return EINVAL;
	}

	srv_addr = argv[0];

	rc = inet_addr_parse(srv_addr, &addr);
	if (rc != EOK) {
		printf(NAME ": Invalid address format '%s'.\n", srv_addr);
		return EINVAL;
	}

	rc = dnsr_set_srvaddr(&addr);
	if (rc != EOK) {
		printf(NAME ": Failed setting server address '%s' (%s)\n",
		    srv_addr, str_error(rc));
		return EIO;
	}

	return EOK;
}

static int dnscfg_unset_ns(int argc, char *argv[])
{
	inet_addr_t addr;
	int rc;

	if (argc > 0) {
		printf(NAME ": Too many arguments.\n");
		print_syntax();
		return EINVAL;
	}

	addr.ipv4 = 0;
	rc = dnsr_set_srvaddr(&addr);
	if (rc != EOK) {
		printf(NAME ": Failed unsetting server address (%s)\n",
		    str_error(rc));
		return EIO;
	}

	return EOK;
}

static int dnscfg_print(void)
{
	inet_addr_t addr;
	char *addr_str;
	int rc;

	rc = dnsr_get_srvaddr(&addr);
	if (rc != EOK) {
		printf(NAME ": Failed getting DNS server address.\n");
		return rc;
	}

	rc = inet_addr_format(&addr, &addr_str);
	if (rc != EOK) {
		printf(NAME ": Out of memory.\n");
		return rc;
	}

	printf("Server: %s\n", addr_str);
	free(addr_str);
	return EOK;
}

int main(int argc, char *argv[])
{
	int rc;

	if (argc < 2) {
		rc = dnscfg_print();
		if (rc != EOK)
			return 1;
		return 0;
	}

	if (str_cmp(argv[1], "set-ns") == 0) {
		rc = dnscfg_set_ns(argc - 2, argv + 2);
		if (rc != EOK)
			return 1;
	} else if (str_cmp(argv[1], "unset-ns") == 0) {
		rc = dnscfg_unset_ns(argc - 2, argv + 2);
		if (rc != EOK)
			return 1;
	} else {
		printf(NAME ": Unknown command '%s'.\n", argv[1]);
		print_syntax();
		return 1;
	}


	return 0;
}

/** @}
 */
