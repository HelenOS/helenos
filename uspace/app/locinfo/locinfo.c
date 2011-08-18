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

/** @addtogroup locinfo
 * @{
 */
/** @file locinfo.c Print information from location service.
 */

#include <errno.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <sys/types.h>
#include <sys/typefmt.h>

#define NAME "locinfo"

int main(int argc, char *argv[])
{
	category_id_t *cat_ids;
	size_t cat_cnt;
	service_id_t *svc_ids;
	size_t svc_cnt;

	size_t i, j;
	char *cat_name;
	char *svc_name;
	int rc;

	rc = loc_get_categories(&cat_ids, &cat_cnt);
	if (rc != EOK) {
		printf(NAME ": Error getting list of categories.\n");
		return 1;
	}

	for (i = 0; i < cat_cnt; i++) {
		rc = loc_category_get_name(cat_ids[i], &cat_name);
		if (rc != EOK)
			cat_name = str_dup("<unknown>");

		if (cat_name == NULL) {
			printf(NAME ": Error allocating memory.\n");
			return 1;
		}

		printf("%s (%" PRIun "):\n", cat_name, cat_ids[i]);

		rc = loc_category_get_svcs(cat_ids[i], &svc_ids, &svc_cnt);
		if (rc != EOK) {
			printf(NAME ": Failed getting list of services in "
			    "category %s, skipping.\n", cat_name);
			free(cat_name);
			continue;
		}

		for (j = 0; j < svc_cnt; j++) {
			rc = loc_service_get_name(svc_ids[j], &svc_name);
			if (rc != EOK) {
				printf(NAME ": Unknown service name (SID %"
				    PRIun ").\n", svc_ids[j]);
				continue;
			}
			printf("\t%s (%" PRIun ")\n", svc_name, svc_ids[j]);
		}

		free(svc_ids);
		free(cat_name);
	}

	free(cat_ids);
	return 0;
}

/** @}
 */
