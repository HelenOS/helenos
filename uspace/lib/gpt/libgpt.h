/*
 * Copyright (c) 2009 Jiri Svoboda
 * Copyright (c) 2011-2013 Dominik Taborsky
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

/** @addtogroup libgpt
 * @{
 */
/** @file
 */

#ifndef LIBGPT_LIBGPT_H_
#define LIBGPT_LIBGPT_H_

#include <loc.h>
#include <sys/types.h>
#include "gpt.h"

/** Block address of GPT header. */
#define GPT_HDR_BA  1

/** Block size of GPT header. */
#define GPT_HDR_BS  1

/** Minimum number of GPT partition entries */
#define GPT_MIN_PART_NUM  128

/** Basic number of GPT partition entries */
#define GPT_BASE_PART_NUM  (GPT_MIN_PART_NUM)

/** How much fill we ignore before resizing partition array */
#define GPT_IGNORE_FILL_NUM  10

/** Unused partition entry */
#define GPT_PTE_UNUSED  0

/** Raw GPT header.
 *
 * Uses more bytes than sizeof(gpt_header_t).
 */
typedef struct {
	gpt_header_t *header;
} gpt_t;

typedef gpt_entry_t gpt_part_t;

typedef struct {
	/** Number of entries */
	size_t fill;
	
	/** Size of the array */
	size_t arr_size;
	
	/** Resizable partition array */
	gpt_part_t *part_array;
} gpt_partitions_t;

typedef struct {
	gpt_t *gpt;
	gpt_partitions_t *parts;
	service_id_t device;
} gpt_label_t;

typedef struct {
	const char *desc;
	const char *guid;
} partition_type_t;

/** GPT header signature ("EFI PART" in ASCII) */
extern const uint8_t efi_signature[8];
extern const uint8_t revision[4];

extern const partition_type_t gpt_ptypes[];

extern gpt_label_t *gpt_alloc_label(void);
extern void gpt_free_label(gpt_label_t *);

extern gpt_t *gpt_alloc_header(size_t);
extern int gpt_read_header(gpt_label_t *, service_id_t);
extern int gpt_write_header(gpt_label_t *, service_id_t);

extern gpt_partitions_t *gpt_alloc_partitions(void);
extern int gpt_read_partitions(gpt_label_t *);
extern int gpt_write_partitions(gpt_label_t *, service_id_t);
extern gpt_part_t *gpt_alloc_partition(void);
extern gpt_part_t *gpt_get_partition(gpt_label_t *);
extern gpt_part_t *gpt_get_partition_at(gpt_label_t *, size_t);
extern int gpt_add_partition(gpt_label_t *, gpt_part_t *);
extern int gpt_remove_partition(gpt_label_t *, size_t);

extern size_t gpt_get_part_type(gpt_part_t *);
extern void gpt_set_part_type(gpt_part_t *, size_t);
extern void gpt_set_start_lba(gpt_part_t *, uint64_t);
extern uint64_t gpt_get_start_lba(gpt_part_t *);
extern void gpt_set_end_lba(gpt_part_t *, uint64_t);
extern uint64_t gpt_get_end_lba(gpt_part_t *);
extern unsigned char *gpt_get_part_name(gpt_part_t *);
extern void gpt_set_part_name(gpt_part_t *, char *, size_t);
extern bool gpt_get_flag(gpt_part_t *, gpt_attr_t);
extern void gpt_set_flag(gpt_part_t *, gpt_attr_t, bool);

extern void gpt_set_random_uuid(uint8_t *);
extern uint64_t gpt_get_next_aligned(uint64_t, unsigned int);


#define gpt_part_foreach(label, iterator) \
	for (gpt_part_t *iterator = (label)->parts->part_array; \
	    iterator < (label)->parts->part_array + (label)->parts->arr_size; \
	    iterator++)

extern void gpt_free_gpt(gpt_t *);
extern void gpt_free_partitions(gpt_partitions_t *);

#endif
