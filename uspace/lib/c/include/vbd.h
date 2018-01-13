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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_VBD_H_
#define LIBC_VBD_H_

#include <async.h>
#include <loc.h>
#include <types/label.h>
#include <offset.h>

/** VBD service */
typedef struct vbd {
	/** VBD session */
	async_sess_t *sess;
} vbd_t;

/** Disk information */
typedef struct {
	/** Label type */
	label_type_t ltype;
	/** Label flags */
	label_flags_t flags;
	/** First block that can be allocated */
	aoff64_t ablock0;
	/** Number of blocks that can be allocated */
	aoff64_t anblocks;
	/** Block size */
	size_t block_size;
	/** Total number of blocks */
	aoff64_t nblocks;
} vbd_disk_info_t;

/** Specification of new partition */
typedef struct {
	/** Partition index */
	int index;
	/** First block */
	aoff64_t block0;
	/** Number of blocks */
	aoff64_t nblocks;
	/** Number of header blocks (EBR for logical partitions) */
	aoff64_t hdr_blocks;
	/** Partition kind */
	label_pkind_t pkind;
	/** Partition type */
	label_ptype_t ptype;
} vbd_part_spec_t;

/** Partition info */
typedef struct {
	/** Partition index */
	int index;
	/** Partition kind */
	label_pkind_t pkind;
	/** First block */
	aoff64_t block0;
	/** Number of blocks */
	aoff64_t nblocks;
	/** Service ID */
	service_id_t svc_id;
} vbd_part_info_t;

typedef sysarg_t vbd_part_id_t;

extern errno_t vbd_create(vbd_t **);
extern void vbd_destroy(vbd_t *);
extern errno_t vbd_get_disks(vbd_t *, service_id_t **, size_t *);
extern errno_t vbd_disk_info(vbd_t *, service_id_t, vbd_disk_info_t *);
extern errno_t vbd_label_create(vbd_t *, service_id_t, label_type_t);
extern errno_t vbd_label_delete(vbd_t *, service_id_t);
extern errno_t vbd_label_get_parts(vbd_t *, service_id_t, service_id_t **,
    size_t *);
extern errno_t vbd_part_get_info(vbd_t *, vbd_part_id_t, vbd_part_info_t *);
extern errno_t vbd_part_create(vbd_t *, service_id_t, vbd_part_spec_t *,
    vbd_part_id_t *);
extern errno_t vbd_part_delete(vbd_t *, vbd_part_id_t);
extern void vbd_pspec_init(vbd_part_spec_t *);
extern errno_t vbd_suggest_ptype(vbd_t *, service_id_t, label_pcnt_t,
    label_ptype_t *);

#endif

/** @}
 */
