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
static fdisk_t *fdisk;

/** Device menu actions */
typedef enum {
	/** Create label */
	devac_create_label,
	/** Delete label */
	devac_delete_label,
	/** Erase disk */
	devac_erase_disk,
	/** Create (primary) partition */
	devac_create_pri_part,
	/** Create extended partition */
	devac_create_ext_part,
	/** Create logical partition */
	devac_create_log_part,
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
	fdisk_dev_info_t *sdev;
	char *scap = NULL;
	char *dtext = NULL;
	service_id_t svcid;
	void *sel;
	int ndevs;
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

	rc = fdisk_dev_list_get(fdisk, &devlist);
	if (rc != EOK) {
		printf("Error getting device list.\n");
		goto error;
	}

	info = fdisk_dev_first(devlist);
	ndevs = 0;
	while (info != NULL) {
		++ndevs;

		rc = fdisk_dev_info_get_svcname(info, &svcname);
		if (rc != EOK) {
			printf("Error getting device service name.\n");
			goto error;
		}

		rc = fdisk_dev_info_capacity(info, &cap);
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

	if (ndevs == 0) {
		printf("No disk devices found.\n");
		rc = ENOENT;
		goto error;
	}

	rc = nchoice_add(choice, "Exit", NULL);
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

	sdev = (fdisk_dev_info_t *)sel;
	if (sdev != NULL) {
		fdisk_dev_info_get_svcid(sdev, &svcid);
	} else {
		svcid = 0;
	}

	fdisk_dev_list_free(devlist);

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
	char *sltype = NULL;
	int i;
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

	for (i = LT_FIRST; i < LT_LIMIT; i++) {
		rc = fdisk_ltype_format(i, &sltype);
		if (rc != EOK)
			goto error;

		rc = nchoice_add(choice, sltype, (void *)(uintptr_t)i);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			printf("Out of memory.\n");
			goto error;
		}

		free(sltype);
		sltype = NULL;
	}

	rc = nchoice_get(choice, &sel);
	if (rc != EOK) {
		printf("Error getting user selection.\n");
		goto error;
	}

	rc = fdisk_label_create(dev, (label_type_t)sel);
	if (rc != EOK) {
		printf("Error creating label.\n");
		goto error;
	}

	nchoice_destroy(choice);
	return EOK;
error:
	free(sltype);
	if (choice != NULL)
		nchoice_destroy(choice);
	return rc;
}

static int fdsk_delete_label(fdisk_dev_t *dev)
{
	int rc;

	rc = fdisk_label_destroy(dev);
	if (rc != EOK) {
		printf("Error deleting label.\n");
		return rc;
	}

	return EOK;
}

static int fdsk_erase_disk(fdisk_dev_t *dev)
{
	int rc;

	rc = fdisk_dev_erase(dev);
	if (rc != EOK) {
		printf("Error erasing disk.\n");
		return rc;
	}

	return EOK;
}

static int fdsk_select_fstype(vol_fstype_t *fstype)
{
	nchoice_t *choice = NULL;
	void *sel;
	char *sfstype;
	int i;
	int rc;

	rc = nchoice_create(&choice);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	rc = nchoice_set_prompt(choice, "Select file system type");
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	for (i = 0; i < VOL_FSTYPE_LIMIT; i++) {
		rc = fdisk_fstype_format(i, &sfstype);
		if (rc != EOK)
			goto error;

		rc = nchoice_add(choice, sfstype, (void *)(uintptr_t)i);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			printf("Out of memory.\n");
			goto error;
		}

		free(sfstype);
		sfstype = NULL;
	}

	rc = nchoice_get(choice, &sel);
	if (rc != EOK) {
		printf("Error getting user selection.\n");
		goto error;
	}

	nchoice_destroy(choice);
	*fstype = (vol_fstype_t)sel;
	return EOK;
error:
	free(sfstype);
	if (choice != NULL)
		nchoice_destroy(choice);
	return rc;
}

static int fdsk_create_part(fdisk_dev_t *dev, label_pkind_t pkind)
{
	int rc;
	fdisk_part_spec_t pspec;
	fdisk_cap_t cap;
	vol_fstype_t fstype = 0;
	tinput_t *tinput = NULL;
	char *scap;

	tinput = tinput_new();
	if (tinput == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = tinput_set_prompt(tinput, "?> ");
	if (rc != EOK)
		goto error;

	while (true) {
		printf("Enter capacity of new partition.\n");
		rc = tinput_read(tinput, &scap);
		if (rc != EOK)
			goto error;

		rc = fdisk_cap_parse(scap, &cap);
		if (rc == EOK)
			break;
	}

	tinput_destroy(tinput);
	tinput = NULL;

	if (pkind != lpk_extended) {
		rc = fdsk_select_fstype(&fstype);
		if (rc != EOK)
			goto error;
	}

	fdisk_pspec_init(&pspec);
	pspec.capacity = cap;
	pspec.pkind = pkind;
	pspec.fstype = fstype;

	rc = fdisk_part_create(dev, &pspec, NULL);
	if (rc != EOK) {
		printf("Error creating partition.\n");
		goto error;
	}

	return EOK;
error:
	if (tinput != NULL)
		tinput_destroy(tinput);
	return rc;
}

static int fdsk_delete_part(fdisk_dev_t *dev)
{
	nchoice_t *choice = NULL;
	fdisk_part_t *part;
	fdisk_part_info_t pinfo;
	char *scap = NULL;
	char *sfstype = NULL;
	char *sdesc = NULL;
	void *sel;
	int rc;

	rc = nchoice_create(&choice);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	rc = nchoice_set_prompt(choice, "Select partition to delete");
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	part = fdisk_part_first(dev);
	while (part != NULL) {
		rc = fdisk_part_get_info(part, &pinfo);
		if (rc != EOK) {
			printf("Error getting partition information.\n");
			goto error;
		}

		rc = fdisk_cap_format(&pinfo.capacity, &scap);
		if (rc != EOK) {
			printf("Out of memory.\n");
			goto error;
		}

		rc = fdisk_fstype_format(pinfo.fstype, &sfstype);
		if (rc != EOK) {
			printf("Out of memory.\n");
			goto error;
		}

		rc = asprintf(&sdesc, "%s, %s", scap, sfstype);
		if (rc < 0) {
			rc = ENOMEM;
			goto error;
		}

		rc = nchoice_add(choice, sdesc, (void *)part);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			printf("Out of memory.\n");
			goto error;
		}

		free(scap);
		scap = NULL;
		free(sfstype);
		sfstype = NULL;
		free(sdesc);
		sdesc = NULL;

		part = fdisk_part_next(part);
	}

	rc = nchoice_get(choice, &sel);
	if (rc != EOK) {
		printf("Error getting user selection.\n");
		goto error;
	}

	rc = fdisk_part_destroy((fdisk_part_t *)sel);
	if (rc != EOK) {
		printf("Error deleting partition.\n");
		return rc;
	}

	nchoice_destroy(choice);
	return EOK;
error:
	free(scap);
	free(sfstype);
	free(sdesc);

	if (choice != NULL)
		nchoice_destroy(choice);
	return rc;
}

/** Device menu */
static int fdsk_dev_menu(fdisk_dev_t *dev)
{
	nchoice_t *choice = NULL;
	fdisk_label_info_t linfo;
	fdisk_part_t *part;
	fdisk_part_info_t pinfo;
	fdisk_cap_t cap;
	fdisk_dev_flags_t dflags;
	char *sltype = NULL;
	char *sdcap = NULL;
	char *scap = NULL;
	char *sfstype = NULL;
	char *svcname = NULL;
	char *spkind;
	int rc;
	int npart;
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

	rc = fdisk_dev_capacity(dev, &cap);
	if (rc != EOK) {
		printf("Error getting device capacity.\n");
		goto error;
	}

	rc = fdisk_cap_format(&cap, &sdcap);
	if (rc != EOK) {
		printf("Out of memory.\n");
		goto error;
	}

	rc = fdisk_dev_get_svcname(dev, &svcname);
	if (rc != EOK) {
		printf("Error getting device service name.\n");
		goto error;
	}

	fdisk_dev_get_flags(dev, &dflags);

	printf("Device: %s, %s\n", sdcap, svcname);
	free(sdcap);
	sdcap = NULL;

	rc = fdisk_label_get_info(dev, &linfo);
	if (rc != EOK) {
		printf("Error getting label information.\n");
		goto error;
	}

	switch (linfo.ltype) {
	case lt_none:
		printf("Disk contains no label.\n");
		break;
	default:
		rc = fdisk_ltype_format(linfo.ltype, &sltype);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			printf("Out of memory.\n");
			goto error;
		}

		printf("Label type: %s\n", sltype);
		free(sltype);
		sltype = NULL;
		break;
	}

	part = fdisk_part_first(dev);
	npart = 0;
	while (part != NULL) {
		++npart;
		rc = fdisk_part_get_info(part, &pinfo);
		if (rc != EOK) {
			printf("Error getting partition information.\n");
			goto error;
		}

		rc = fdisk_cap_format(&pinfo.capacity, &scap);
		if (rc != EOK) {
			printf("Out of memory.\n");
			goto error;
		}

		rc = fdisk_fstype_format(pinfo.fstype, &sfstype);
		if (rc != EOK) {
			printf("Out of memory.\n");
			goto error;
		}

		if (linfo.ltype == lt_none)
			printf("Entire disk: %s", scap);
		else
			printf("Partition %d: %s", npart, scap);

		if ((linfo.flags & lf_ext_supp) != 0) {
			rc = fdisk_pkind_format(pinfo.pkind, &spkind);
			if (rc != EOK) {
				printf("\nOut of memory.\n");
				goto error;
			}

			printf(", %s", spkind);
			free(spkind);
		}

		if (pinfo.pkind != lpk_extended) {
			switch (pinfo.pcnt) {
			case vpc_empty:
				printf(", Empty");
				break;
			case vpc_fs:
				printf(", %s", sfstype);
				break;
			case vpc_unknown:
				printf(", Unknown");
				break;
			}
		}

		printf("\n");

		free(scap);
		scap = NULL;
		free(sfstype);
		sfstype = NULL;

		part = fdisk_part_next(part);
	}

	rc = nchoice_set_prompt(choice, "Select action");
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	if ((linfo.flags & lf_ext_supp) != 0) {
		if ((linfo.flags & lf_can_create_pri) != 0) {
			rc = nchoice_add(choice, "Create primary "
			    "partition",
			    (void *)devac_create_pri_part);
			if (rc != EOK) {
				assert(rc == ENOMEM);
				printf("Out of memory.\n");
				goto error;
			}
		}

		if ((linfo.flags & lf_can_create_ext) != 0) {
			rc = nchoice_add(choice, "Create extended "
			    "partition",
			    (void *)devac_create_ext_part);
			if (rc != EOK) {
				assert(rc == ENOMEM);
				printf("Out of memory.\n");
				goto error;
			}
		}

		if ((linfo.flags & lf_can_create_log) != 0) {
			rc = nchoice_add(choice, "Create logical "
			    "partition",
			    (void *)devac_create_log_part);
			if (rc != EOK) {
				assert(rc == ENOMEM);
				printf("Out of memory.\n");
				goto error;
			}
		}
	} else { /* (linfo.flags & lf_ext_supp) == 0 */
		if ((linfo.flags & lf_can_create_pri) != 0) {
			rc = nchoice_add(choice, "Create partition",
			    (void *)devac_create_pri_part);
			if (rc != EOK) {
				assert(rc == ENOMEM);
					printf("Out of memory.\n");
					goto error;
				}
		}
	}

	if ((linfo.flags & lf_can_delete_part) != 0) {
		rc = nchoice_add(choice, "Delete partition",
		    (void *)devac_delete_part);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			printf("Out of memory.\n");
			goto error;
		}
	}

	if ((dflags & fdf_can_create_label) != 0) {
		rc = nchoice_add(choice, "Create label",
		    (void *)devac_create_label);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			printf("Out of memory.\n");
			goto error;
		}
	}

	if ((dflags & fdf_can_delete_label) != 0) {
		rc = nchoice_add(choice, "Delete label",
		    (void *)devac_delete_label);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			printf("Out of memory.\n");
			goto error;
		}
	}

	if ((dflags & fdf_can_erase_dev) != 0) {
		rc = nchoice_add(choice, "Erase disk",
		    (void *)devac_erase_disk);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			printf("Out of memory.\n");
			goto error;
		}
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
		(void) fdsk_create_label(dev);
		break;
	case devac_delete_label:
		(void) fdsk_delete_label(dev);
		break;
	case devac_erase_disk:
		(void) fdsk_erase_disk(dev);
		break;
	case devac_create_pri_part:
		(void) fdsk_create_part(dev, lpk_primary);
		break;
	case devac_create_ext_part:
		(void) fdsk_create_part(dev, lpk_extended);
		break;
	case devac_create_log_part:
		(void) fdsk_create_part(dev, lpk_logical);
		break;
	case devac_delete_part:
		(void) fdsk_delete_part(dev);
		break;
	case devac_exit:
		quit = true;
		break;
	}

	nchoice_destroy(choice);
	return EOK;
error:
	free(sdcap);
	free(scap);
	free(sfstype);
	free(svcname);
	if (choice != NULL)
		nchoice_destroy(choice);
	return rc;
}

int main(int argc, char *argv[])
{
	service_id_t svcid;
	fdisk_dev_t *dev;
	int rc;

	rc = fdisk_create(&fdisk);
	if (rc != EOK) {
		printf("Error initializing Fdisk.\n");
		return 1;
	}

	rc = fdsk_dev_sel_choice(&svcid);
	if (rc != EOK)
		return 1;

	if (svcid == 0)
		return 0;

	rc = fdisk_dev_open(fdisk, svcid, &dev);
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
	fdisk_destroy(fdisk);

	return 0;
}


/** @}
 */
