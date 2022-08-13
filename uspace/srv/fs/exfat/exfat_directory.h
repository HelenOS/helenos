/*
 * SPDX-FileCopyrightText: 2011 Oleg Romanenko
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup exfat
 * @{
 */

#ifndef EXFAT_EXFAT_DIRECTORY_H_
#define EXFAT_EXFAT_DIRECTORY_H_

#include <stdint.h>
#include "exfat.h"
#include "exfat_fat.h"
#include "exfat_dentry.h"

typedef struct {
	/* Directory data */
	exfat_bs_t *bs;
	exfat_node_t *nodep;
	service_id_t service_id;
	uint32_t blocks;
	uint32_t bnum;
	aoff64_t pos;
	block_t *b;
	bool last;
	bool fragmented;
	exfat_cluster_t firstc;
} exfat_directory_t;

extern void exfat_directory_init(exfat_directory_t *);
extern errno_t exfat_directory_open(exfat_node_t *, exfat_directory_t *);
extern errno_t exfat_directory_open_parent(exfat_directory_t *, service_id_t,
    exfat_cluster_t, bool);
extern errno_t exfat_directory_close(exfat_directory_t *);

extern errno_t exfat_directory_next(exfat_directory_t *);
extern errno_t exfat_directory_prev(exfat_directory_t *);
extern errno_t exfat_directory_seek(exfat_directory_t *, aoff64_t);
extern errno_t exfat_directory_get(exfat_directory_t *, exfat_dentry_t **);
extern errno_t exfat_directory_find(exfat_directory_t *, exfat_dentry_clsf_t,
    exfat_dentry_t **);
extern errno_t exfat_directory_find_continue(exfat_directory_t *,
    exfat_dentry_clsf_t, exfat_dentry_t **);

extern errno_t exfat_directory_read_file(exfat_directory_t *, char *, size_t,
    exfat_file_dentry_t *, exfat_stream_dentry_t *);
extern errno_t exfat_directory_read_vollabel(exfat_directory_t *, char *, size_t);
extern errno_t exfat_directory_sync_file(exfat_directory_t *, exfat_file_dentry_t *,
    exfat_stream_dentry_t *);
extern errno_t exfat_directory_write_file(exfat_directory_t *, const char *);
extern errno_t exfat_directory_erase_file(exfat_directory_t *, aoff64_t);

extern errno_t exfat_directory_expand(exfat_directory_t *);
extern errno_t exfat_directory_lookup_free(exfat_directory_t *, size_t);
extern errno_t exfat_directory_print(exfat_directory_t *);

#endif

/**
 * @}
 */
