/*
 * SPDX-FileCopyrightText: 2011 Oleg Romanenko
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup fat
 * @{
 */

#ifndef FAT_FAT_DIRECTORY_H_
#define FAT_FAT_DIRECTORY_H_

#include <stdint.h>
#include "fat.h"
#include "fat_dentry.h"

#define FAT_MAX_SFN 9999

typedef struct {
	/* Directory data */
	fat_bs_t *bs;
	fat_node_t *nodep;
	uint32_t blocks;
	uint32_t bnum;
	aoff64_t pos;
	block_t *b;
	bool last;
} fat_directory_t;

extern errno_t fat_directory_open(fat_node_t *, fat_directory_t *);
extern errno_t fat_directory_close(fat_directory_t *);

extern errno_t fat_directory_next(fat_directory_t *);
extern errno_t fat_directory_prev(fat_directory_t *);
extern errno_t fat_directory_seek(fat_directory_t *, aoff64_t);
extern errno_t fat_directory_get(fat_directory_t *, fat_dentry_t **);

extern errno_t fat_directory_read(fat_directory_t *, char *, fat_dentry_t **);
extern errno_t fat_directory_write(fat_directory_t *, const char *, fat_dentry_t *);
extern errno_t fat_directory_erase(fat_directory_t *);
extern errno_t fat_directory_lookup_name(fat_directory_t *, const char *,
    fat_dentry_t **);
extern bool fat_directory_is_sfn_exist(fat_directory_t *, fat_dentry_t *);

extern errno_t fat_directory_lookup_free(fat_directory_t *, size_t);
extern errno_t fat_directory_write_dentry(fat_directory_t *, fat_dentry_t *);
extern errno_t fat_directory_create_sfn(fat_directory_t *, fat_dentry_t *,
    const char *);
extern errno_t fat_directory_expand(fat_directory_t *);
extern errno_t fat_directory_vollabel_get(fat_directory_t *, char *);

#endif

/**
 * @}
 */
