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
 * @brief Df utility.
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <getopt.h>
#include <sys/statfs.h>
#include <errno.h>
#include <adt/list.h>
#include <vfs/vfs.h>

#define NAME  "df"

#define HEADER_TABLE 	"Filesystem     %4u-blocks           Used      Available Used%% Mounted on"
#define HEADER_TABLE_HR "Filesystem           Size           Used      Available Used%% Mounted on"

#define PERCENTAGE(x, tot) (tot ? (100ULL * (x) / (tot)) : 0)
#define FSBK_TO_BK(x, fsbk, bk) \
	(((fsbk) != 0 && (fsbk) < (bk)) ? \
		(unsigned long long) ((x) / ((bk) / (fsbk))) : \
		(unsigned long long) ((x) * ((fsbk) / (bk))))

static unsigned int unit_size;
static unsigned int human_readable;

static void size_to_human_readable(char *buf, uint64_t bytes);
static void print_header(void);
static void print_statfs(struct statfs *, char *, char *);
static void print_usage(void);

int main(int argc, char *argv[])
{
	int optres, errflg = 0;
	struct statfs st;

	unit_size = 512;
	human_readable = 0;

	/******************************************/
	/*   Parse command line options...        */
	/******************************************/
	while ((optres = getopt(argc, argv, ":uhb:")) != -1) {
		switch(optres) {
		case 'u':
			print_usage();
			return 0;

		case 'h':
			human_readable = 1;
			break;

		case 'b':
			str_uint32_t(optarg, NULL, 0, 0, &unit_size);
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
	get_mtab_list(&mtab_list);

	print_header();
	list_foreach(mtab_list, link, mtab_ent_t, mtab_ent) {
		statfs(mtab_ent->mp, &st);
		print_statfs(&st, mtab_ent->fs_name, mtab_ent->mp);
	}

	putchar('\n');
	return 0;
}

static void size_to_human_readable(char *buf, uint64_t bytes)
{
	const char *units = "BkMGTPEZY";
	int i = 0;

	while (bytes >= 1024) {
		bytes /= 1024;
		i++;
	}

	snprintf(buf, 6, "%4llu%c", (unsigned long long) bytes, units[i]);
}

static void print_header(void)
{
	if (human_readable)
		printf(HEADER_TABLE_HR);
	else
		printf(HEADER_TABLE, unit_size);

	putchar('\n');
}

static void print_statfs(struct statfs *st, char *name, char *mountpoint)
{
	uint64_t const used_blocks = st->f_blocks - st->f_bfree;
	unsigned const perc_used = PERCENTAGE(used_blocks, st->f_blocks);

	printf("%10s", name);

	if (human_readable) {
		char tmp[1024];

		/* Print size */
		size_to_human_readable(tmp, st->f_blocks *  st->f_bsize);
		printf(" %14s", tmp);

		/* Number of used blocks */
		size_to_human_readable(tmp, used_blocks  *  st->f_bsize);
		printf(" %14s", tmp);

		/* Number of available blocks */
		size_to_human_readable(tmp, st->f_bfree *  st->f_bsize);
		printf(" %14s", tmp);

		/* Percentage of used blocks */
		printf(" %4u%%", perc_used);

		/* Mount point */
		printf(" %s\n", mountpoint);
	} else {
		/* Blocks / Used blocks / Available blocks / Used% / Mounted on */
		printf(" %15llu %14llu %14llu %4u%% %s\n", 
		    FSBK_TO_BK(st->f_blocks, st->f_bsize, unit_size),
		    FSBK_TO_BK(used_blocks, st->f_bsize, unit_size),
		    FSBK_TO_BK(st->f_bfree, st->f_bsize, unit_size),
		    perc_used,
		    mountpoint);
	}

}

static void print_usage(void)
{
	printf("syntax: %s [-u] [-h] [-b <size>] \n", NAME);
	printf("  u : Show usage.\n");
	printf("  h : \"Human-readable\" output.\n");
	printf("  b : Scale block sizes by selected size.\n");
	printf("\n");
}

/** @}
 */
