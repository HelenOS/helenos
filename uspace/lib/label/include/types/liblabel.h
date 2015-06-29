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

typedef struct {
	/** Disk contents */
	label_disk_cnt_t dcnt;
	/** Label type */
	label_type_t ltype;
} label_info_t;

typedef struct {
	/** Address of first block */
	aoff64_t block0;
	/** Number of blocks */
	aoff64_t nblocks;
} label_part_info_t;

/** Partition */
typedef struct {
	/** Containing label */
	struct label *label;
	/** Link to label_t.parts */
	link_t llabel;
} label_part_t;

/** Specification of new partition */
typedef struct {
} label_part_spec_t;

/** Label instance */
typedef struct label {
	/** Partitions */
	list_t parts; /* of label_part_t */
} label_t;

#endif

/** @}
 */
