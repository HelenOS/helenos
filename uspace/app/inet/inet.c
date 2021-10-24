/*
 * Copyright (c) 2021 Jiri Svoboda
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

/** @addtogroup inet
 * @{
 */
/** @file Internet configuration utility.
 *
 * Controls the internet service (@c inet).
 */

#include <errno.h>
#include <inet/addr.h>
#include <inet/eth_addr.h>
#include <inet/inetcfg.h>
#include <io/table.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <str.h>
#include <str_error.h>

#define NAME  "inet"

static void print_syntax(void)
{
	printf("%s: Internet configuration utility.\n", NAME);
	printf("Syntax:\n");
	printf("  %s list-addr\n", NAME);
	printf("  %s create-addr <addr>/<width> <link-name> <addr-name>\n", NAME);
	printf("  %s delete-addr <link-name> <addr-name>\n", NAME);
	printf("  %s list-sr\n", NAME);
	printf("  %s create-sr <dest-addr>/<width> <router-addr> <route-name>\n", NAME);
	printf("  %s delete-sr <route-name>\n", NAME);
	printf("  %s list-link\n", NAME);
}

static errno_t addr_create_static(int argc, char *argv[])
{
	char *aobj_name;
	char *addr_spec;
	char *link_name;

	inet_naddr_t naddr;
	sysarg_t link_id;
	sysarg_t addr_id;
	errno_t rc;

	if (argc < 3) {
		printf(NAME ": Missing arguments.\n");
		print_syntax();
		return EINVAL;
	}

	if (argc > 3) {
		printf(NAME ": Too many arguments.\n");
		print_syntax();
		return EINVAL;
	}

	addr_spec = argv[0];
	link_name = argv[1];
	aobj_name = argv[2];

	rc = loc_service_get_id(link_name, &link_id, 0);
	if (rc != EOK) {
		printf(NAME ": Service '%s' not found: %s.\n", link_name, str_error(rc));
		return ENOENT;
	}

	rc = inet_naddr_parse(addr_spec, &naddr, NULL);
	if (rc != EOK) {
		printf(NAME ": Invalid network address format '%s'.\n",
		    addr_spec);
		return EINVAL;
	}

	rc = inetcfg_addr_create_static(aobj_name, &naddr, link_id, &addr_id);
	if (rc != EOK) {
		printf(NAME ": Failed creating static address '%s' (%s)\n",
		    aobj_name, str_error(rc));
		return EIO;
	}

	return EOK;
}

static errno_t addr_delete(int argc, char *argv[])
{
	char *aobj_name;
	char *link_name;
	sysarg_t link_id;
	sysarg_t addr_id;
	errno_t rc;

	if (argc < 2) {
		printf(NAME ": Missing arguments.\n");
		print_syntax();
		return EINVAL;
	}

	if (argc > 2) {
		printf(NAME ": Too many arguments.\n");
		print_syntax();
		return EINVAL;
	}

	link_name = argv[0];
	aobj_name = argv[1];

	rc = loc_service_get_id(link_name, &link_id, 0);
	if (rc != EOK) {
		printf(NAME ": Service '%s' not found: %s.\n", link_name,
		    str_error(rc));
		return ENOENT;
	}

	rc = inetcfg_addr_get_id(aobj_name, link_id, &addr_id);
	if (rc != EOK) {
		printf(NAME ": Address '%s' not found: %s.\n", aobj_name,
		    str_error(rc));
		return ENOENT;
	}

	rc = inetcfg_addr_delete(addr_id);
	if (rc != EOK) {
		printf(NAME ": Failed deleting address '%s': %s\n", aobj_name,
		    str_error(rc));
		return EIO;
	}

	return EOK;
}

static errno_t sroute_create(int argc, char *argv[])
{
	char *dest_str;
	char *router_str;
	char *route_name;

	inet_naddr_t dest;
	inet_addr_t router;
	sysarg_t sroute_id;
	errno_t rc;

	if (argc < 3) {
		printf(NAME ": Missing arguments.\n");
		print_syntax();
		return EINVAL;
	}

	if (argc > 3) {
		printf(NAME ": Too many arguments.\n");
		print_syntax();
		return EINVAL;
	}

	dest_str   = argv[0];
	router_str = argv[1];
	route_name = argv[2];

	rc = inet_naddr_parse(dest_str, &dest, NULL);
	if (rc != EOK) {
		printf(NAME ": Invalid network address format '%s'.\n",
		    dest_str);
		return EINVAL;
	}

	rc = inet_addr_parse(router_str, &router, NULL);
	if (rc != EOK) {
		printf(NAME ": Invalid address format '%s'.\n", router_str);
		return EINVAL;
	}

	rc = inetcfg_sroute_create(route_name, &dest, &router, &sroute_id);
	if (rc != EOK) {
		printf(NAME ": Failed creating static route '%s': %s\n",
		    route_name, str_error(rc));
		return EIO;
	}

	return EOK;
}

static errno_t sroute_delete(int argc, char *argv[])
{
	char *route_name;
	sysarg_t sroute_id;
	errno_t rc;

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

	route_name = argv[0];

	rc = inetcfg_sroute_get_id(route_name, &sroute_id);
	if (rc != EOK) {
		printf(NAME ": Static route '%s' not found: %s.\n",
		    route_name, str_error(rc));
		return ENOENT;
	}

	rc = inetcfg_sroute_delete(sroute_id);
	if (rc != EOK) {
		printf(NAME ": Failed deleting static route '%s': %s\n",
		    route_name, str_error(rc));
		return EIO;
	}

	return EOK;
}

static errno_t addr_list(void)
{
	sysarg_t *addr_list;
	inet_addr_info_t ainfo;
	inet_link_info_t linfo;
	table_t *table = NULL;

	size_t count;
	size_t i;
	errno_t rc;
	char *astr = NULL;

	ainfo.name = NULL;
	linfo.name = NULL;

	rc = inetcfg_get_addr_list(&addr_list, &count);
	if (rc != EOK) {
		printf(NAME ": Failed getting address list.\n");
		return rc;
	}

	rc = table_create(&table);
	if (rc != EOK) {
		printf("Memory allocation failed.\n");
		goto out;
	}

	table_header_row(table);
	table_printf(table, "Addr/Width\t" "Link-Name\t" "Addr-Name\t"
	    "Def-MTU\n");

	for (i = 0; i < count; i++) {
		rc = inetcfg_addr_get(addr_list[i], &ainfo);
		if (rc != EOK) {
			printf("Failed getting properties of address %zu.\n",
			    (size_t)addr_list[i]);
			ainfo.name = NULL;
			continue;
		}

		rc = inetcfg_link_get(ainfo.ilink, &linfo);
		if (rc != EOK) {
			printf("Failed getting properties of link %zu.\n",
			    (size_t)ainfo.ilink);
			linfo.name = NULL;
			continue;
		}

		rc = inet_naddr_format(&ainfo.naddr, &astr);
		if (rc != EOK) {
			printf("Memory allocation failed.\n");
			astr = NULL;
			goto out;
		}

		table_printf(table, "%s\t" "%s\t" "%s\t" "%zu\n", astr,
		    linfo.name, ainfo.name, linfo.def_mtu);

		free(ainfo.name);
		free(linfo.name);
		free(astr);

		ainfo.name = NULL;
		linfo.name = NULL;
		astr = NULL;
	}

	if (count != 0) {
		rc = table_print_out(table, stdout);
		if (rc != EOK) {
			printf("Error printing table.\n");
			goto out;
		}
	}

	rc = EOK;
out:
	table_destroy(table);
	if (ainfo.name != NULL)
		free(ainfo.name);
	if (linfo.name != NULL)
		free(linfo.name);
	if (astr != NULL)
		free(astr);

	free(addr_list);

	return rc;
}

static errno_t link_list(void)
{
	sysarg_t *link_list = NULL;
	inet_link_info_t linfo;
	table_t *table = NULL;
	eth_addr_str_t saddr;

	size_t count;
	size_t i;
	errno_t rc;

	rc = inetcfg_get_link_list(&link_list, &count);
	if (rc != EOK) {
		printf(NAME ": Failed getting link list.\n");
		return rc;
	}

	rc = table_create(&table);
	if (rc != EOK) {
		printf("Memory allocation failed.\n");
		goto out;
	}

	table_header_row(table);
	table_printf(table, "Link-layer Address\t" "Link-Name\t" "Def-MTU\n");

	for (i = 0; i < count; i++) {
		rc = inetcfg_link_get(link_list[i], &linfo);
		if (rc != EOK) {
			printf("Failed getting properties of link %zu.\n",
			    (size_t)link_list[i]);
			continue;
		}

		eth_addr_format(&linfo.mac_addr, &saddr);
		table_printf(table, "%s\t %s\t %zu\n", saddr.str, linfo.name,
		    linfo.def_mtu);

		free(linfo.name);

		linfo.name = NULL;
	}

	if (count != 0) {
		rc = table_print_out(table, stdout);
		if (rc != EOK) {
			printf("Error printing table.\n");
			goto out;
		}
	}

	rc = EOK;
out:
	table_destroy(table);
	free(link_list);

	return rc;
}

static errno_t sroute_list(void)
{
	sysarg_t *sroute_list = NULL;
	inet_sroute_info_t srinfo;
	table_t *table = NULL;

	size_t count;
	size_t i;
	errno_t rc;
	char *dest_str = NULL;
	char *router_str = NULL;

	srinfo.name = NULL;

	rc = inetcfg_get_sroute_list(&sroute_list, &count);
	if (rc != EOK) {
		printf(NAME ": Failed getting address list.\n");
		return rc;
	}

	rc = table_create(&table);
	if (rc != EOK) {
		printf("Memory allocation failed.\n");
		goto out;
	}

	table_header_row(table);
	table_printf(table, "Dest/Width\t" "Router-Addr\t" "Route-Name\n");

	for (i = 0; i < count; i++) {
		rc = inetcfg_sroute_get(sroute_list[i], &srinfo);
		if (rc != EOK) {
			printf("Failed getting properties of static route %zu.\n",
			    (size_t)sroute_list[i]);
			srinfo.name = NULL;
			continue;
		}

		rc = inet_naddr_format(&srinfo.dest, &dest_str);
		if (rc != EOK) {
			printf("Memory allocation failed.\n");
			dest_str = NULL;
			goto out;
		}

		rc = inet_addr_format(&srinfo.router, &router_str);
		if (rc != EOK) {
			printf("Memory allocation failed.\n");
			router_str = NULL;
			goto out;
		}

		table_printf(table, "%s\t" "%s\t" "%s\n", dest_str, router_str,
		    srinfo.name);

		free(srinfo.name);
		free(dest_str);
		free(router_str);

		router_str = NULL;
		srinfo.name = NULL;
		dest_str = NULL;
	}

	if (count != 0) {
		rc = table_print_out(table, stdout);
		if (rc != EOK) {
			printf("Error printing table.\n");
			goto out;
		}
	}

	rc = EOK;
out:
	table_destroy(table);
	if (srinfo.name != NULL)
		free(srinfo.name);
	if (dest_str != NULL)
		free(dest_str);
	if (router_str != NULL)
		free(router_str);

	free(sroute_list);

	return rc;
}

int main(int argc, char *argv[])
{
	errno_t rc;

	rc = inetcfg_init();
	if (rc != EOK) {
		printf(NAME ": Failed connecting to internet service: %s.\n",
		    str_error(rc));
		return 1;
	}

	if (argc < 2 || str_cmp(argv[1], "-h") == 0) {
		print_syntax();
		return 0;
	}

	if (str_cmp(argv[1], "list-addr") == 0) {
		rc = addr_list();
		if (rc != EOK)
			return 1;
	} else if (str_cmp(argv[1], "create-addr") == 0) {
		rc = addr_create_static(argc - 2, argv + 2);
		if (rc != EOK)
			return 1;
	} else if (str_cmp(argv[1], "delete-addr") == 0) {
		rc = addr_delete(argc - 2, argv + 2);
		if (rc != EOK)
			return 1;
	} else if (str_cmp(argv[1], "list-sr") == 0) {
		rc = sroute_list();
		if (rc != EOK)
			return 1;
	} else if (str_cmp(argv[1], "create-sr") == 0) {
		rc = sroute_create(argc - 2, argv + 2);
		if (rc != EOK)
			return 1;
	} else if (str_cmp(argv[1], "delete-sr") == 0) {
		rc = sroute_delete(argc - 2, argv + 2);
		if (rc != EOK)
			return 1;
	} else if (str_cmp(argv[1], "list-link") == 0) {
		rc = link_list();
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
