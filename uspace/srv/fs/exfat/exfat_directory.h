/*
 * Copyright (c) 2011 Oleg Romanenko
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

/** @addtogroup fs
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
