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
#include <sys/types.h>
#include <vol.h>

typedef struct label label_t;
typedef struct label_info label_info_t;
typedef struct label_part label_part_t;
typedef struct label_part_info label_part_info_t;
typedef struct label_part_spec label_part_spec_t;

/** Ops for individual label type */
typedef struct {
	int (*open)(service_id_t, label_t **);
	int (*create)(service_id_t, label_t **);
	void (*close)(label_t *);
	int (*destroy)(label_t *);
	int (*get_info)(label_t *, label_info_t *);
	label_part_t *(*part_first)(label_t *);
	label_part_t *(*part_next)(label_part_t *);
	void (*part_get_info)(label_part_t *, label_part_info_t *);
	int (*part_create)(label_t *, label_part_spec_t *, label_part_t **);
	int (*part_destroy)(label_part_t *);
} label_ops_t;

struct label_info {
	/** Disk contents */
	label_disk_cnt_t dcnt;
	/** Label type */
	label_type_t ltype;
	/** First block that can be allocated */
	aoff64_t ablock0;
	/** Number of blocks that can be allocated */
	aoff64_t anblocks;
};

struct label_part_info {
	/** Partition index */
	int index;
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
	link_t llabel;
	/** Index */
	int index;
	/** First block */
	aoff64_t block0;
	/** Number of blocks */
	aoff64_t nblocks;
	/** Partition type */
	uint64_t ptype;
};

/** Specification of new partition */
struct label_part_spec {
	/** Partition index */
	int index;
	/** First block */
	aoff64_t block0;
	/** Number of blocks */
	aoff64_t nblocks;
	/** Partition type */
	uint64_t ptype;
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

/** Label instance */
struct label {
	/** Label type ops */
	label_ops_t *ops;
	/** Label type */
	label_type_t ltype;
	/** Block device service ID */
	service_id_t svc_id;
	/** Partitions */
	list_t parts; /* of label_part_t */
	/** First block that can be allocated */
	aoff64_t ablock0;
	/** Number of blocks that can be allocated */
	aoff64_t anblocks;
	/** Number of primary partition entries */
	int pri_entries;
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
