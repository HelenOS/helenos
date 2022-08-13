/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
#include <stdint.h>
#include <str.h>
#include <str_error.h>

#define NAME "dnscfg"

static void print_syntax(void)
{
	printf("Syntax:\n");
	printf("\t%s get-ns\n", NAME);
	printf("\t%s set-ns <server-addr>\n", NAME);
	printf("\t%s unset-ns\n", NAME);
}

static errno_t dnscfg_set_ns(int argc, char *argv[])
{
	if (argc < 1) {
		printf("%s: Missing arguments.\n", NAME);
		print_syntax();
		return EINVAL;
	}

	if (argc > 1) {
		printf("%s: Too many arguments.\n", NAME);
		print_syntax();
		return EINVAL;
	}

	char *srv_addr  = argv[0];

	inet_addr_t addr;
	errno_t rc = inet_addr_parse(srv_addr, &addr, NULL);

	if (rc != EOK) {
		printf("%s: Invalid address format '%s'.\n", NAME, srv_addr);
		return rc;
	}

	rc = dnsr_set_srvaddr(&addr);
	if (rc != EOK) {
		printf("%s: Failed setting nameserver address '%s' (%s)\n",
		    NAME, srv_addr, str_error(rc));
		return rc;
	}

	return EOK;
}

static errno_t dnscfg_unset_ns(void)
{
	inet_addr_t addr;
	inet_addr_any(&addr);

	errno_t rc = dnsr_set_srvaddr(&addr);
	if (rc != EOK) {
		printf("%s: Failed unsetting server address (%s)\n",
		    NAME, str_error(rc));
		return rc;
	}

	return EOK;
}

static errno_t dnscfg_print(void)
{
	inet_addr_t addr;
	errno_t rc = dnsr_get_srvaddr(&addr);
	if (rc != EOK) {
		printf("%s: Failed getting DNS server address.\n", NAME);
		return rc;
	}

	char *addr_str;
	rc = inet_addr_format(&addr, &addr_str);
	if (rc != EOK) {
		printf("%s: Out of memory.\n", NAME);
		return rc;
	}

	printf("Nameserver: %s\n", addr_str);
	free(addr_str);
	return EOK;
}

int main(int argc, char *argv[])
{
	if ((argc < 2) || (str_cmp(argv[1], "get-ns") == 0))
		return dnscfg_print();
	else if (str_cmp(argv[1], "set-ns") == 0)
		return dnscfg_set_ns(argc - 2, argv + 2);
	else if (str_cmp(argv[1], "unset-ns") == 0)
		return dnscfg_unset_ns();
	else {
		printf("%s: Unknown command '%s'.\n", NAME, argv[1]);
		print_syntax();
		return 1;
	}

	return 0;
}

/** @}
 */
