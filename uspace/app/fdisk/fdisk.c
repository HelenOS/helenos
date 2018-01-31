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

#include <cap.h>
#include <errno.h>
#include <nchoice.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <fdisk.h>
#include <str.h>

#define NO_LABEL_CAPTION "(No name)"

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

static errno_t fdsk_pcnt_fs_format(vol_part_cnt_t pcnt, vol_fstype_t fstype,
    char **rstr)
{
	errno_t rc;
	char *s;

	switch (pcnt) {
	case vpc_empty:
		s = str_dup("Empty");
		if (s == NULL)
			return ENOMEM;
		break;
	case vpc_fs:
		rc = fdisk_fstype_format(fstype, &s);
		if (rc != EOK)
			return ENOMEM;
		break;
	case vpc_unknown:
		s = str_dup("Unknown");
		if (s == NULL)
			return ENOMEM;
		break;
	}

	*rstr = s;
	return EOK;
}

/** Confirm user selection. */
static errno_t fdsk_confirm(const char *msg, bool *rconfirm)
{
	tinput_t *tinput = NULL;
	char *answer;
	errno_t rc;

	tinput = tinput_new();
	if (tinput == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = tinput_set_prompt(tinput, "y/n> ");
	if (rc != EOK)
		goto error;

	while (true) {
		printf("%s\n", msg);

		rc = tinput_read(tinput, &answer);
		if (rc == ENOENT) {
			*rconfirm = false;
			free(answer);
			break;
		}

		if (rc != EOK)
			goto error;

		if (str_cmp(answer, "y") == 0) {
			*rconfirm = true;
			free(answer);
			break;
		} else if (str_cmp(answer, "n") == 0) {
			*rconfirm = false;
			free(answer);
			break;
		}
	}

	tinput_destroy(tinput);
	return EOK;
error:
	if (tinput != NULL)
		tinput_destroy(tinput);
	return rc;
}

/** Device selection */
static errno_t fdsk_dev_sel_choice(service_id_t *rsvcid)
{
	fdisk_dev_list_t *devlist = NULL;
	fdisk_dev_info_t *info;
	nchoice_t *choice = NULL;
	char *svcname = NULL;
	cap_spec_t cap;
	fdisk_dev_info_t *sdev;
	char *scap = NULL;
	char *dtext = NULL;
	service_id_t svcid;
	void *sel;
	int ndevs;
	errno_t rc;

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
		rc = fdisk_dev_info_get_svcname(info, &svcname);
		if (rc != EOK) {
			fdisk_dev_info_get_svcid(info, &svcid);
			printf("Error getting device service name "
			    "(service ID %zu).\n", svcid);
			info = fdisk_dev_next(info);
			continue;
		}

		rc = fdisk_dev_info_capacity(info, &cap);
		if (rc != EOK) {
			printf("Error getting device capacity "
			    "(device %s).\n", svcname);
			info = fdisk_dev_next(info);
			continue;
		}

		cap_simplify(&cap);

		rc = cap_format(&cap, &scap);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			printf("Out of memory.\n");
			goto error;
		}

		int ret = asprintf(&dtext, "%s (%s)", svcname, scap);
		if (ret < 0) {
			rc = ENOMEM;
			printf("Out of memory.\n");
			goto error;
		}

		free(svcname);
		svcname = NULL;
		free(scap);
		scap = NULL;

		rc = nchoice_add(choice, dtext, info, 0);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			printf("Out of memory.\n");
			goto error;
		}

		++ndevs;

		free(dtext);
		dtext = NULL;

		info = fdisk_dev_next(info);
	}

	if (ndevs == 0) {
		printf("No disk devices found.\n");
		rc = ENOENT;
		goto error;
	}

	rc = nchoice_add(choice, "Exit", NULL, 0);
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
	assert(rc != EOK);
	*rsvcid = 0;
	if (devlist != NULL)
		fdisk_dev_list_free(devlist);
	if (choice != NULL)
		nchoice_destroy(choice);
	free(dtext);
	free(svcname);
	free(scap);
	return rc;
}

static errno_t fdsk_create_label(fdisk_dev_t *dev)
{
	nchoice_t *choice = NULL;
	void *sel;
	char *sltype = NULL;
	int i;
	errno_t rc;

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

		rc = nchoice_add(choice, sltype, (void *)(uintptr_t)i,
		    i == LT_DEFAULT);
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

static errno_t fdsk_delete_label(fdisk_dev_t *dev)
{
	bool confirm;
	errno_t rc;

	rc = fdsk_confirm("Warning. Any data on disk will be lost. "
	    "Really delete label?", &confirm);
	if (rc != EOK) {
		printf("Error getting user confirmation.\n");
		return rc;
	}

	if (!confirm)
		return EOK;

	rc = fdisk_label_destroy(dev);
	if (rc != EOK) {
		printf("Error deleting label.\n");
		return rc;
	}

	return EOK;
}

static errno_t fdsk_erase_disk(fdisk_dev_t *dev)
{
	bool confirm;
	errno_t rc;

	rc = fdsk_confirm("Warning. Any data on disk will be lost. "
	    "Really erase disk?", &confirm);
	if (rc != EOK) {
		printf("Error getting user confirmation.\n");
		return rc;
	}

	if (!confirm)
		return EOK;

	rc = fdisk_dev_erase(dev);
	if (rc != EOK) {
		printf("Error erasing disk.\n");
		return rc;
	}

	return EOK;
}

static errno_t fdsk_select_fstype(vol_fstype_t *fstype)
{
	nchoice_t *choice = NULL;
	void *sel;
	char *sfstype;
	int i;
	errno_t rc;

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

		rc = nchoice_add(choice, sfstype, (void *)(uintptr_t)i,
		    i == VOL_FSTYPE_DEFAULT);
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

static errno_t fdsk_create_part(fdisk_dev_t *dev, label_pkind_t pkind)
{
	errno_t rc;
	fdisk_part_spec_t pspec;
	cap_spec_t cap;
	cap_spec_t mcap;
	vol_label_supp_t vlsupp;
	vol_fstype_t fstype = 0;
	tinput_t *tinput = NULL;
	fdisk_spc_t spc;
	char *scap;
	char *smcap = NULL;
	char *label = NULL;

	if (pkind == lpk_logical)
		spc = spc_log;
	else
		spc = spc_pri;

	rc = fdisk_part_get_max_avail(dev, spc, &mcap);
	if (rc != EOK) {
		rc = EIO;
		goto error;
	}

	cap_simplify(&mcap);

	rc = cap_format(&mcap, &smcap);
	if (rc != EOK) {
		rc = ENOMEM;
		goto error;
	}

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
		rc = tinput_read_i(tinput, smcap, &scap);
		if (rc != EOK)
			goto error;

		rc = cap_parse(scap, &cap);
		if (rc == EOK)
			break;
	}

	tinput_destroy(tinput);
	tinput = NULL;
	free(smcap);
	smcap = NULL;

	if (pkind != lpk_extended) {
		rc = fdsk_select_fstype(&fstype);
		if (rc != EOK)
			goto error;
	}

	fdisk_get_vollabel_support(dev, fstype, &vlsupp);
	if (vlsupp.supported) {
		tinput = tinput_new();
		if (tinput == NULL) {
			rc = ENOMEM;
			goto error;
		}

		rc = tinput_set_prompt(tinput, "?> ");
		if (rc != EOK)
			goto error;

		/* Ask for volume label */
		printf("Enter volume label for new partition.\n");
		rc = tinput_read_i(tinput, "New volume", &label);
		if (rc != EOK)
			goto error;

	    	tinput_destroy(tinput);
		tinput = NULL;
	}

	fdisk_pspec_init(&pspec);
	pspec.capacity = cap;
	pspec.pkind = pkind;
	pspec.fstype = fstype;
	pspec.label = label;

	rc = fdisk_part_create(dev, &pspec, NULL);
	if (rc != EOK) {
		printf("Error creating partition.\n");
		goto error;
	}

	free(label);
	return EOK;
error:
	free(smcap);
	free(label);
	if (tinput != NULL)
		tinput_destroy(tinput);
	return rc;
}

static errno_t fdsk_delete_part(fdisk_dev_t *dev)
{
	nchoice_t *choice = NULL;
	fdisk_part_t *part;
	fdisk_part_info_t pinfo;
	char *scap = NULL;
	char *spkind = NULL;
	char *sfstype = NULL;
	char *sdesc = NULL;
	const char *label;
	bool confirm;
	void *sel;
	errno_t rc;

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

		cap_simplify(&pinfo.capacity);

		rc = cap_format(&pinfo.capacity, &scap);
		if (rc != EOK) {
			printf("Out of memory.\n");
			goto error;
		}

		rc = fdisk_pkind_format(pinfo.pkind, &spkind);
		if (rc != EOK) {
			printf("\nOut of memory.\n");
			goto error;
		}

		if (pinfo.pkind != lpk_extended) {
			rc = fdsk_pcnt_fs_format(pinfo.pcnt, pinfo.fstype, &sfstype);
			if (rc != EOK) {
				printf("Out of memory.\n");
				goto error;
			}

			if (str_size(pinfo.label) > 0)
				label = pinfo.label;
			else
				label = "(No name)";

			int ret = asprintf(&sdesc, "%s %s, %s, %s", label,
			    scap, spkind, sfstype);
			if (ret < 0) {
				rc = ENOMEM;
				goto error;
			}

		} else {
			int ret = asprintf(&sdesc, "%s, %s", scap, spkind);
			if (ret < 0) {
				rc = ENOMEM;
				goto error;
			}
		}

		rc = nchoice_add(choice, sdesc, (void *)part, 0);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			printf("Out of memory.\n");
			goto error;
		}

		free(scap);
		scap = NULL;
		free(spkind);
		spkind = NULL;
		free(sfstype);
		sfstype = NULL;
		free(sdesc);
		sdesc = NULL;

		part = fdisk_part_next(part);
	}

	rc = nchoice_add(choice, "Cancel", NULL, 0);
	if (rc != EOK) {
		assert(rc == ENOMEM);
		printf("Out of memory.\n");
		goto error;
	}

	rc = nchoice_get(choice, &sel);
	if (rc == ENOENT)
		return EOK;
	if (rc != EOK) {
		printf("Error getting user selection.\n");
		goto error;
	}


	nchoice_destroy(choice);
	choice = NULL;

	if (sel == NULL)
		return EOK;

	rc = fdsk_confirm("Warning. Any data in partition will be lost. "
	    "Really delete partition?", &confirm);
	if (rc != EOK) {
		printf("Error getting user confirmation.\n");
		goto error;
	}

	if (!confirm)
		return EOK;

	rc = fdisk_part_destroy((fdisk_part_t *)sel);
	if (rc != EOK) {
		printf("Error deleting partition.\n");
		return rc;
	}

	return EOK;
error:
	free(scap);
	free(spkind);
	free(sfstype);
	free(sdesc);

	if (choice != NULL)
		nchoice_destroy(choice);
	return rc;
}

/** Device menu */
static errno_t fdsk_dev_menu(fdisk_dev_t *dev)
{
	nchoice_t *choice = NULL;
	fdisk_label_info_t linfo;
	fdisk_part_t *part;
	fdisk_part_info_t pinfo;
	cap_spec_t cap;
	cap_spec_t mcap;
	fdisk_dev_flags_t dflags;
	char *sltype = NULL;
	char *sdcap = NULL;
	char *scap = NULL;
	char *smcap = NULL;
	char *sfstype = NULL;
	char *svcname = NULL;
	char *spkind;
	const char *label;
	errno_t rc;
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

	cap_simplify(&cap);

	rc = cap_format(&cap, &sdcap);
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

	printf("Device: %s (%s)\n", svcname, sdcap);
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

		cap_simplify(&pinfo.capacity);

		rc = cap_format(&pinfo.capacity, &scap);
		if (rc != EOK) {
			printf("Out of memory.\n");
			goto error;
		}

		rc = fdsk_pcnt_fs_format(pinfo.pcnt, pinfo.fstype, &sfstype);
		if (rc != EOK) {
			printf("Out of memory.\n");
			goto error;
		}

		if (str_size(pinfo.label) > 0)
			label = pinfo.label;
		else
			label = "(No name)";

		if (linfo.ltype == lt_none)
			printf("Entire disk: %s %s", label, scap);
		else
			printf("Partition %d: %s %s", npart, label, scap);

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
			printf(", %s", sfstype);
		}

		printf("\n");

		free(scap);
		scap = NULL;
		free(sfstype);
		sfstype = NULL;

		part = fdisk_part_next(part);
	}

	/* Display available space */
	if ((linfo.flags & lf_can_create_pri) != 0) {
		rc = fdisk_part_get_max_avail(dev, spc_pri, &mcap);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}

		cap_simplify(&mcap);

		rc = cap_format(&mcap, &smcap);
		if (rc != EOK) {
			rc = ENOMEM;
			goto error;
		}

		if ((linfo.flags & lf_ext_supp) != 0)
			printf("Maximum free primary block: %s\n", smcap);
		else
			printf("Maximum free block: %s\n", smcap);

		free(smcap);
		smcap = NULL;

		rc = fdisk_part_get_tot_avail(dev, spc_pri, &mcap);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}

		cap_simplify(&mcap);

		rc = cap_format(&mcap, &smcap);
		if (rc != EOK) {
			rc = ENOMEM;
			goto error;
		}

		if ((linfo.flags & lf_ext_supp) != 0)
			printf("Total free primary space: %s\n", smcap);
		else
			printf("Total free space: %s\n", smcap);

		free(smcap);
		smcap = NULL;
	}

	/* Display available space */
	if ((linfo.flags & lf_can_create_log) != 0) {
		rc = fdisk_part_get_max_avail(dev, spc_log, &mcap);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}

		cap_simplify(&mcap);

		rc = cap_format(&mcap, &smcap);
		if (rc != EOK) {
			rc = ENOMEM;
			goto error;
		}

		printf("Maximum free logical block: %s\n", smcap);
		free(smcap);
		smcap = NULL;

		rc = fdisk_part_get_tot_avail(dev, spc_log, &mcap);
		if (rc != EOK) {
			rc = EIO;
			goto error;
		}

		cap_simplify(&mcap);

		rc = cap_format(&mcap, &smcap);
		if (rc != EOK) {
			rc = ENOMEM;
			goto error;
		}

		printf("Total free logical space: %s\n", smcap);
		free(smcap);
		smcap = NULL;
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
			    (void *)devac_create_pri_part, 0);
			if (rc != EOK) {
				assert(rc == ENOMEM);
				printf("Out of memory.\n");
				goto error;
			}
		}

		if ((linfo.flags & lf_can_create_ext) != 0) {
			rc = nchoice_add(choice, "Create extended "
			    "partition",
			    (void *)devac_create_ext_part, 0);
			if (rc != EOK) {
				assert(rc == ENOMEM);
				printf("Out of memory.\n");
				goto error;
			}
		}

		if ((linfo.flags & lf_can_create_log) != 0) {
			rc = nchoice_add(choice, "Create logical "
			    "partition",
			    (void *)devac_create_log_part, 0);
			if (rc != EOK) {
				assert(rc == ENOMEM);
				printf("Out of memory.\n");
				goto error;
			}
		}
	} else { /* (linfo.flags & lf_ext_supp) == 0 */
		if ((linfo.flags & lf_can_create_pri) != 0) {
			rc = nchoice_add(choice, "Create partition",
			    (void *)devac_create_pri_part, 0);
			if (rc != EOK) {
				assert(rc == ENOMEM);
					printf("Out of memory.\n");
					goto error;
				}
		}
	}

	if ((linfo.flags & lf_can_delete_part) != 0) {
		rc = nchoice_add(choice, "Delete partition",
		    (void *)devac_delete_part, 0);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			printf("Out of memory.\n");
			goto error;
		}
	}

	if ((dflags & fdf_can_create_label) != 0) {
		rc = nchoice_add(choice, "Create label",
		    (void *)devac_create_label, 0);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			printf("Out of memory.\n");
			goto error;
		}
	}

	if ((dflags & fdf_can_delete_label) != 0) {
		rc = nchoice_add(choice, "Delete label",
		    (void *)devac_delete_label, 0);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			printf("Out of memory.\n");
			goto error;
		}
	}

	if ((dflags & fdf_can_erase_dev) != 0) {
		rc = nchoice_add(choice, "Erase disk",
		    (void *)devac_erase_disk, 0);
		if (rc != EOK) {
			assert(rc == ENOMEM);
			printf("Out of memory.\n");
			goto error;
		}
	}

	rc = nchoice_add(choice, "Exit", (void *)devac_exit, 0);
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
	free(smcap);
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
	errno_t rc;

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
