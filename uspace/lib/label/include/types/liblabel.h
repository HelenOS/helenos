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

/** @addtogroup liblabel
 * @{
 */
/**
 * @file Disk label library types.
 */

#ifndef LIBLABEL_TYPES_H_
#define LIBLABEL_TYPES_H_

#include <adt/list.h>
#include <types/label.h>
#include <offset.h>
#include <stddef.h>
#include <vol.h>
#include <uuid.h>

typedef struct label label_t;
typedef struct label_info label_info_t;
typedef struct label_part label_part_t;
typedef struct label_part_info label_part_info_t;
typedef struct label_part_spec label_part_spec_t;
typedef struct label_bd label_bd_t;
typedef struct label_bd_ops label_bd_ops_t;

/** Ops for individual label type */
typedef struct {
	errno_t (*open)(label_bd_t *, label_t **);
	errno_t (*create)(label_bd_t *, label_t **);
	void (*close)(label_t *);
	errno_t (*destroy)(label_t *);
	errno_t (*get_info)(label_t *, label_info_t *);
	label_part_t *(*part_first)(label_t *);
	label_part_t *(*part_next)(label_part_t *);
	void (*part_get_info)(label_part_t *, label_part_info_t *);
	errno_t (*part_create)(label_t *, label_part_spec_t *, label_part_t **);
	errno_t (*part_destroy)(label_part_t *);
	errno_t (*suggest_ptype)(label_t *, label_pcnt_t, label_ptype_t *);
} label_ops_t;

struct label_info {
	/** Label type */
	label_type_t ltype;
	/** Label flags */
	label_flags_t flags;
	/** First block that can be allocated */
	aoff64_t ablock0;
	/** Number of blocks that can be allocated */
	aoff64_t anblocks;
};

struct label_part_info {
	/** Partition index */
	int index;
	/** Partition kind */
	label_pkind_t pkind;
	/** Address of first block */
	aoff64_t block0;
	/** Number of blocks */
	aoff64_t nblocks;
};

/** Partition */
struct label_part {
	/** Containing label */
	struct label *label;
	/** Link to label_t.parts */
	link_t lparts;
	/** Link to label_t.pri_parts */
	link_t lpri;
	/** Link to label_t.log_parts */
	link_t llog;
	/** Index */
	int index;
	/** Number of EBR blocks preceding a logical partition */
	aoff64_t hdr_blocks;
	/** First block */
	aoff64_t block0;
	/** Number of blocks */
	aoff64_t nblocks;
	/** Partition type */
	label_ptype_t ptype;
	/** Partition UUID */
	uuid_t part_uuid;
};

/** Specification of new partition */
struct label_part_spec {
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
};

typedef struct {
	uint64_t hdr_ba[2];
	uint32_t hdr_size;
	uint64_t ptable_ba[2];
	uint64_t pt_blocks;
	size_t esize;
	uint32_t pt_crc;
} label_gpt_t;

typedef struct {
} label_mbr_t;

/** Block device operations */
struct label_bd_ops {
	/** Get block size */
	errno_t (*get_bsize)(void *, size_t *);
	/** Get number of blocks */
	errno_t (*get_nblocks)(void *, aoff64_t *);
	/** Read blocks */
	errno_t (*read)(void *, aoff64_t, size_t, void *);
	/** Write blocks */
	errno_t (*write)(void *, aoff64_t, size_t, const void *);
};

/** Block device */
struct label_bd {
	/** Ops structure */
	label_bd_ops_t *ops;
	/** Argument */
	void *arg;
};

/** Label instance */
struct label {
	/** Label type ops */
	label_ops_t *ops;
	/** Label type */
	label_type_t ltype;
	/** Block device */
	label_bd_t bd;
	/** Partitions */
	list_t parts; /* of label_part_t */
	/** Primary partitions */
	list_t pri_parts; /* of label_part_t */
	/** Logical partitions */
	list_t log_parts; /* of label_part_t */
	/** First block that can be allocated */
	aoff64_t ablock0;
	/** Number of blocks that can be allocated */
	aoff64_t anblocks;
	/** Number of primary partition entries */
	int pri_entries;
	/** Extended partition or NULL if there is none */
	label_part_t *ext_part;
	/** Block size */
	size_t block_size;
	union {
		label_gpt_t gpt;
		label_mbr_t mbr;
	} lt;
};

#endif

/** @}
 */
