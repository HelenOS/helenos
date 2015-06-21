/*
 * Copyright (c) 2015 Jiri Svoboda
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

/** @addtogroup fdisk
 * @brief Disk management tool
 * @{
 */
/**
 * @file
 */

#include <errno.h>
#include <nchoice.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <fdisk.h>

static bool quit = false;

/** Device menu actions */
typedef enum {
	/** Create label */
	devac_create_label,
	/** Delete label */
	devac_delete_label,
	/** Create partition */
	devac_create_part,
	/** Delete partition */
	devac_delete_part,
	/** Exit */
	devac_exit
} devac_t;

/** Device selection */
static int fdsk_dev_sel_choice(service_id_t *rsvcid)
{
	fdisk_dev_list_t *devlist = NULL;
	fdisk_dev_info_t *info;
	nchoice_t *choice = NULL;
	char *svcname = NULL;
	fdisk_cap_t cap;
	char *scap = NULL;
	char *dtext = NULL;
	service_id_t svcid;
	void *sel;
	int rc;

	rc = nchoice_create(&choice);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	rc = nchoice_set_prompt(choice, "Select device");
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	rc = fdisk_dev_list_get(&devlist);
	if (rc != EOK) {
		printf("Error getting device list.\n");
		goto error;
	}

	info = fdisk_dev_first(devlist);
	while (info != NULL) {
		rc = fdisk_dev_get_svcname(info, &svcname);
		if (rc != EOK) {
			printf("Error getting device service name.\n");
			goto error;
		}

		rc = fdisk_dev_capacity(info, &cap);
		if (rc != EOK) {
			printf("Error getting device capacity.\n");
			goto error;
		}

		rc = fdisk_cap_format(&cap, &scap);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			printf("Out of memory.\n");
			goto error;
		}

		rc = asprintf(&dtext, "%s (%s)", svcname, scap);
		if (rc < 0) {
			rc = ENOMEM;
			printf("Out of memory.\n");
			goto error;
		}

		free(svcname);
		svcname = NULL;
		free(scap);
		scap = NULL;

		rc = nchoice_add(choice, dtext, info);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			printf("Out of memory.\n");
			goto error;
		}

		free(dtext);
		dtext = NULL;

		info = fdisk_dev_next(info);
	}

	rc = nchoice_get(choice, &sel);
	if (rc != EOK) {
		printf("Error getting user selection.\n");
		return rc;
	}

	fdisk_dev_get_svcid((fdisk_dev_info_t *)sel, &svcid);
	fdisk_dev_list_free(devlist);

	printf("Selected dev with sid=%zu\n", svcid);

	nchoice_destroy(choice);
	*rsvcid = svcid;
	return EOK;
error:
	if (devlist != NULL)
		fdisk_dev_list_free(devlist);
	if (choice != NULL)
		nchoice_destroy(choice);
	free(dtext);
	free(svcname);
	free(scap);
	return rc;
}

static int fdsk_create_label(fdisk_dev_t *dev)
{
	nchoice_t *choice = NULL;
	void *sel;
	int rc;

	rc = nchoice_create(&choice);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	rc = nchoice_set_prompt(choice, "Select label type");
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	rc = nchoice_add(choice, "GPT", (void *)fdl_gpt);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	rc = nchoice_add(choice, "MBR", (void *)fdl_mbr);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	rc = nchoice_get(choice, &sel);
	if (rc != EOK) {
		printf("Error getting user selection.\n");
		goto error;
	}

	rc = fdisk_label_create(dev, (fdisk_label_type_t)sel);
	if (rc != EOK) {
		printf("Error creating label.\n");
		goto error;
	}

	nchoice_destroy(choice);
	return EOK;
error:
	if (choice != NULL)
		nchoice_destroy(choice);
	return rc;
}

/** Device menu */
static int fdsk_dev_menu(fdisk_dev_t *dev)
{
	nchoice_t *choice = NULL;
	fdisk_label_info_t linfo;
	char *sltype = NULL;
	int rc;
	void *sel;

	rc = nchoice_create(&choice);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	rc = nchoice_set_prompt(choice, "Select action");
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	rc = fdisk_label_get_info(dev, &linfo);
	if (rc != EOK) {
		printf("Error getting label information.\n");
		goto error;
	}

	rc = fdisk_ltype_format(linfo.ltype, &sltype);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	printf("Label type: %s\n", sltype);
	free(sltype);
	sltype = NULL;

	rc = nchoice_set_prompt(choice, "Select action");
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	if (linfo.ltype == fdl_none) {
		rc = nchoice_add(choice, "Create label",
		    (void *)devac_create_label);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			printf("Out of memory.\n");
			goto error;
		}
	} else {
		rc = nchoice_add(choice, "Delete label",
		    (void *)devac_delete_label);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			printf("Out of memory.\n");
			goto error;
		}
	}

	rc = nchoice_add(choice, "Create partition",
	    (void *)devac_create_part);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	rc = nchoice_add(choice, "Delete partition",
	    (void *)devac_delete_part);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	rc = nchoice_add(choice, "Exit", (void *)devac_exit);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	rc = nchoice_get(choice, &sel);
	if (rc != EOK) {
		printf("Error getting user selection.\n");
		return rc;
	}

	switch ((devac_t)sel) {
	case devac_create_label:
		rc = fdsk_create_label(dev);
		if (rc != EOK)
			goto error;
		break;
	case devac_delete_label:
		rc = fdisk_label_destroy(dev);
		if (rc != EOK)
			goto error;
		break;
	case devac_create_part:
		break;
	case devac_delete_part:
		break;
	case devac_exit:
		quit = true;
		break;
	}

	nchoice_destroy(choice);
	return EOK;
error:
	if (choice != NULL)
		nchoice_destroy(choice);
	return rc;
}

int main(int argc, char *argv[])
{
	service_id_t svcid;
	fdisk_dev_t *dev;
	int rc;

	rc = fdsk_dev_sel_choice(&svcid);
	if (rc != EOK)
		return 1;

	rc = fdisk_dev_open(svcid, &dev);
	if (rc != EOK) {
		printf("Error opening device.\n");
		return 1;
	}

	while (!quit) {
		rc = fdsk_dev_menu(dev);
		if (rc != EOK) {
			fdisk_dev_close(dev);
			return 1;
		}
	}

	fdisk_dev_close(dev);

	return 0;
}


/** @}
 */
