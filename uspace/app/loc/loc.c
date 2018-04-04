/*
 * Copyright (c) 2011 Jiri Svoboda
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

/** @addtogroup loc
 * @{
 */
/** @file loc.c Print information from location service.
 */

#include <errno.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <str.h>

#define NAME "loc"

static errno_t show_cat(const char *cat_name, category_id_t cat_id)
{
	service_id_t *svc_ids;
	size_t svc_cnt;
	char *svc_name;
	char *server_name;
	errno_t rc;
	size_t j;

	printf("%s:\n", cat_name);

	rc = loc_category_get_svcs(cat_id, &svc_ids, &svc_cnt);
	if (rc != EOK) {
		printf(NAME ": Failed getting list of services in "
		    "category %s, skipping.\n", cat_name);
		return rc;
	}

	for (j = 0; j < svc_cnt; j++) {
		rc = loc_service_get_name(svc_ids[j], &svc_name);
		if (rc != EOK) {
			printf(NAME ": Unknown service name (SID %"
			    PRIun ").\n", svc_ids[j]);
			continue;
		}

		rc = loc_service_get_server_name(svc_ids[j], &server_name);
		if (rc != EOK && rc != EINVAL) {
			free(svc_name);
			printf(NAME ": Unknown service name (SID %"
			    PRIun ").\n", svc_ids[j]);
			continue;
		}

		if (rc == EOK)
			printf("\t%s : %s\n", svc_name, server_name);
		else
			printf("\t%s\n", svc_name);

		free(svc_name);
		free(server_name);
	}

	free(svc_ids);
	return EOK;
}

static errno_t list_svcs_by_cat(void)
{
	category_id_t *cat_ids;
	size_t cat_cnt;

	size_t i;
	char *cat_name;
	errno_t rc;

	rc = loc_get_categories(&cat_ids, &cat_cnt);
	if (rc != EOK) {
		printf(NAME ": Error getting list of categories.\n");
		return rc;
	}

	for (i = 0; i < cat_cnt; i++) {
		rc = loc_category_get_name(cat_ids[i], &cat_name);
		if (rc != EOK)
			cat_name = str_dup("<unknown>");

		if (cat_name == NULL) {
			printf(NAME ": Error allocating memory.\n");
			free(cat_ids);
			return rc;
		}

		rc = show_cat(cat_name, cat_ids[i]);
		(void) rc;

		free(cat_name);
	}

	free(cat_ids);
	return EOK;
}

static void print_syntax(void)
{
	printf("syntax:\n"
	    "\t" NAME "                      List categories and services "
	    "they contain\n"
	    "\t" NAME " show-cat <category>  List services in category\n");
}

int main(int argc, char *argv[])
{
	errno_t rc;
	char *cmd;
	char *cat_name;
	category_id_t cat_id;

	if (argc <= 1) {
		rc = list_svcs_by_cat();
		if (rc != EOK)
			return 1;
		return 0;
	}

	cmd = argv[1];
	if (str_cmp(cmd, "show-cat") == 0) {
		if (argc < 3) {
			printf("Argument missing.\n");
			print_syntax();
			return 1;
		}

		cat_name = argv[2];
		rc = loc_category_get_id(cat_name, &cat_id, 0);
		if (rc != EOK) {
			printf("Error looking up category '%s'.\n", cat_name);
			return 1;
		}

		rc = show_cat(cat_name, cat_id);
		if (rc != EOK)
			return 1;
	} else {
		printf("Invalid command '%s'\n", cmd);
		print_syntax();
		return 1;
	}

	return 0;
}

/** @}
 */
