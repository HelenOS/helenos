/*
 * Copyright (c) 2013 Manuele Conti
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

/** @addtogroup df
 * @brief Print amounts of free and used disk space.
 * @{
 */
/**
 * @file
 */

#include <cap.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <str_error.h>
#include <getopt.h>
#include <errno.h>
#include <adt/list.h>
#include <vfs/vfs.h>
#include <vfs/vfs_mtab.h>

#define NAME  "df"

#define HEADER_TABLE     "Filesystem           Size           Used      Available Used%% Mounted on"
#define HEADER_TABLE_BLK "Filesystem  Blk. Size     Total        Used   Available Used%% Mounted on"

#define PERCENTAGE(x, tot) (tot ? (100ULL * (x) / (tot)) : 0)

static bool display_blocks;

static errno_t size_to_human_readable(uint64_t, size_t, char **);
static void print_header(void);
static errno_t print_statfs(vfs_statfs_t *, char *, char *);
static void print_usage(void);

int main(int argc, char *argv[])
{
	int optres, errflg = 0;
	vfs_statfs_t st;
	errno_t rc;

	display_blocks = false;

	/* Parse command-line options */
	while ((optres = getopt(argc, argv, ":ubh")) != -1) {
		switch (optres) {
		case 'h':
			print_usage();
			return 0;

		case 'b':
			display_blocks = true;
			break;

		case ':':
			fprintf(stderr, "Option -%c requires an operand\n",
			    optopt);
			errflg++;
			break;

		case '?':
			fprintf(stderr, "Unrecognized option: -%c\n", optopt);
			errflg++;
			break;

		default:
			fprintf(stderr,
			    "Unknown error while parsing command line options");
			errflg++;
			break;
		}
	}

	if (optind > argc) {
		fprintf(stderr, "Too many input parameters\n");
		errflg++;
	}

	if (errflg) {
		print_usage();
		return 1;
	}

	LIST_INITIALIZE(mtab_list);
	vfs_get_mtab_list(&mtab_list);

	print_header();
	list_foreach(mtab_list, link, mtab_ent_t, mtab_ent) {
		if (vfs_statfs_path(mtab_ent->mp, &st) == 0) {
			rc = print_statfs(&st, mtab_ent->fs_name, mtab_ent->mp);
			if (rc != EOK)
				return 1;
		} else {
			fprintf(stderr, "Cannot get information for '%s' (%s).\n",
			    mtab_ent->mp, str_error(errno));
		}
	}

	putchar('\n');
	return 0;
}

static errno_t size_to_human_readable(uint64_t nblocks, size_t block_size, char **rptr)
{
	cap_spec_t cap;

	cap_from_blocks(nblocks, block_size, &cap);
	cap_simplify(&cap);
	return cap_format(&cap, rptr);
}

static void print_header(void)
{
	if (!display_blocks)
		printf(HEADER_TABLE);
	else
		printf(HEADER_TABLE_BLK);

	putchar('\n');
}

static errno_t print_statfs(vfs_statfs_t *st, char *name, char *mountpoint)
{
	uint64_t const used_blocks = st->f_blocks - st->f_bfree;
	unsigned const perc_used = PERCENTAGE(used_blocks, st->f_blocks);
	char *str;
	errno_t rc;

	printf("%10s", name);

	if (!display_blocks) {
		/* Print size */
		rc = size_to_human_readable(st->f_blocks, st->f_bsize, &str);
		if (rc != EOK)
			goto error;
		printf(" %14s", str);
		free(str);

		/* Number of used blocks */
		rc = size_to_human_readable(used_blocks, st->f_bsize, &str);
		if (rc != EOK)
			goto error;
		printf(" %14s", str);
		free(str);

		/* Number of available blocks */
		rc = size_to_human_readable(st->f_bfree, st->f_bsize, &str);
		if (rc != EOK)
			goto error;
		printf(" %14s", str);
		free(str);

		/* Percentage of used blocks */
		printf(" %4u%%", perc_used);

		/* Mount point */
		printf(" %s\n", mountpoint);
	} else {
		/* Block size / Blocks / Used blocks / Available blocks / Used% / Mounted on */
		printf(" %10" PRIu32 " %9" PRIu64 " %11" PRIu64 " %11" PRIu64 " %4u%% %s\n",
		    st->f_bsize, st->f_blocks, used_blocks, st->f_bfree,
		    perc_used, mountpoint);
	}

	return EOK;
error:
	printf("\nError: Out of memory.\n");
	return ENOMEM;
}

static void print_usage(void)
{
	printf("Syntax: %s [<options>] \n", NAME);
	printf("Options:\n");
	printf("  -h Print help\n");
	printf("  -b Print exact block sizes and numbers\n");
}

/** @}
 */
