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

/** @addtogroup inetcfg
 * @{
 */
/** @file Internet configuration utility.
 *
 * Controls the internet service (@c inet).
 */

#include <async.h>
#include <errno.h>
#include <inet/inetcfg.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <str_error.h>
#include <sys/types.h>

#define NAME "inetcfg"

static void print_syntax(void)
{
	printf("syntax:\n");
	printf("\t" NAME " create <addr>/<width> <link-name> <addr-name>\n");
	printf("\t" NAME " delete <link-name> <addr-name>\n");
}

static int naddr_parse(const char *text, inet_naddr_t *naddr)
{
	unsigned long a[4], bits;
	char *cp = (char *)text;
	int i;

	for (i = 0; i < 3; i++) {
		a[i] = strtoul(cp, &cp, 10);
		if (*cp != '.')
			return EINVAL;
		++cp;
	}

	a[3] = strtoul(cp, &cp, 10);
	if (*cp != '/')
		return EINVAL;
	++cp;

	bits = strtoul(cp, &cp, 10);
	if (*cp != '\0')
		return EINVAL;

	naddr->ipv4 = 0;
	for (i = 0; i < 4; i++) {
		if (a[i] > 255)
			return EINVAL;
		naddr->ipv4 = (naddr->ipv4 << 8) | a[i];
	}

	if (bits < 1 || bits > 31)
		return EINVAL;

	naddr->bits = bits;
	return EOK;
}

static int naddr_format(inet_naddr_t *naddr, char **bufp)
{
	int rc;

	rc = asprintf(bufp, "%d.%d.%d.%d/%d", naddr->ipv4 >> 24,
	    (naddr->ipv4 >> 16) & 0xff, (naddr->ipv4 >> 8) & 0xff,
	    naddr->ipv4 & 0xff, naddr->bits);

	if (rc < 0)
		return ENOMEM;

	return EOK;
}

static int addr_create_static(int argc, char *argv[])
{
	char *aobj_name;
	char *addr_spec;
	char *link_name;

	inet_naddr_t naddr;
	sysarg_t link_id;
	sysarg_t addr_id;
	int rc;

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
		printf(NAME ": Service '%s' not found (%d).\n", link_name, rc);
		return ENOENT;
	}

	rc = naddr_parse(addr_spec, &naddr);
	if (rc != EOK) {
		printf(NAME ": Invalid address format '%s'.\n", addr_spec);
		return EINVAL;
	}

	rc = inetcfg_addr_create_static(aobj_name, &naddr, link_id, &addr_id);
	if (rc != EOK) {
		printf(NAME ": Failed creating static address '%s' (%d)\n",
		    aobj_name, rc);
		return EIO;
	}

	return EOK;
}

static int addr_delete(int argc, char *argv[])
{
	char *aobj_name;
	char *link_name;
	sysarg_t link_id;
	sysarg_t addr_id;
	int rc;

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
		printf(NAME ": Service '%s' not found (%d).\n", link_name, rc);
		return ENOENT;
	}

	rc = inetcfg_addr_get_id(aobj_name, link_id, &addr_id);
	if (rc != EOK) {
		printf(NAME ": Address '%s' not found (%d).\n", aobj_name, rc);
		return ENOENT;
	}

	rc = inetcfg_addr_delete(addr_id);
	if (rc != EOK) {
		printf(NAME ": Failed deleting address '%s' (%d)\n", aobj_name,
		    rc);
		return EIO;
	}

	return EOK;
}

static int addr_list(void)
{
	sysarg_t *addr_list;
	inet_addr_info_t ainfo;
	inet_link_info_t linfo;

	size_t count;
	size_t i;
	int rc;
	char *astr;

	rc = inetcfg_get_addr_list(&addr_list, &count);
	if (rc != EOK) {
		printf(NAME ": Failed getting address list.\n");
		return rc;
	}

	for (i = 0; i < count; i++) {
		rc = inetcfg_addr_get(addr_list[i], &ainfo);
		if (rc != EOK) {
			printf("Failed getting properties of address %zu.\n",
			    (size_t)addr_list[i]);
			continue;
		}

		rc = inetcfg_link_get(ainfo.ilink, &linfo);
		if (rc != EOK) {
			printf("Failed getting properties of link %zu.\n",
			    (size_t)ainfo.ilink);
			continue;
		}

		rc = naddr_format(&ainfo.naddr, &astr);
		if (rc != EOK) {
			printf("Memory allocation failed.\n");
			goto out;
		}

		printf("%s %s %s\n", astr, linfo.name,
		    ainfo.name);

		free(astr);
		free(ainfo.name);
		free(linfo.name);
	}
out:
	free(addr_list);

	return EOK;
}

int main(int argc, char *argv[])
{
	int rc;

	rc = inetcfg_init();
	if (rc != EOK) {
		printf(NAME ": Failed connecting to internet service (%d).\n",
		    rc);
		return 1;
	}

	if (argc < 2) {
		rc = addr_list();
		if (rc != EOK)
			return 1;
		return 0;
	}

	if (str_cmp(argv[1], "create") == 0) {
		rc = addr_create_static(argc - 2, argv + 2);
		if (rc != EOK)
			return 1;
	} else if (str_cmp(argv[1], "delete") == 0) {
		rc = addr_delete(argc - 2, argv + 2);
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
