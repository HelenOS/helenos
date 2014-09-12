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

/** @addtogroup libmbr
 * @{
 */
/** @file
 */

#ifndef LIBMBR_LIBMBR_H_
#define LIBMBR_LIBMBR_H_

#include <adt/list.h>
#include <loc.h>
#include <sys/types.h>
#include "mbr.h"

/*
 * WARNING: When changing both header and partitions, write first header,
 * then partitions. The MBR headers' raw_data is not updated to follow
 * partition changes.
 *
 * NOTE: Writing partitions writes the complete header as well.
 */

typedef enum {
	/** Other flags unknown - saving previous state */
	/** Bootability */
	ST_BOOT = 7,
	/** Logical partition, 0 = primary, 1 = logical*/
	ST_LOGIC = 8
} mbr_flags_t;

typedef enum {
	/** No error */
	ERR_OK = 0,
	/** All primary partitions already present */
	ERR_PRIMARY_FULL,
	/** Extended partition already present */
	ERR_EXTENDED_PRESENT,
	/** No extended partition present */
	ERR_NO_EXTENDED,
	/** Partition overlapping */
	ERR_OVERLAP,
	/** Partition out of bounds */
	ERR_OUT_BOUNDS,
	/** No space left for EBR */
	ERR_NO_EBR,
	/** Out of memory */
	ERR_NOMEM,
	/** Libblock error */
	ERR_LIBBLOCK,
} mbr_err_val;

/** MBR header */
typedef struct {
	/** Raw access to data */
	br_block_t raw_data;
} mbr_t;

/** Partition */
typedef struct mbr_part {
	/** The link in the doubly-linked list */
	link_t link;
	/** Partition type */
	uint8_t type;
	/** Flags */
	uint16_t status;
	/** Address of first block */
	uint32_t start_addr;
	/** Number of blocks */
	uint32_t length;
	/** Points to Extended Boot Record of logical partition */
	br_block_t *ebr;
	/** EBR address */
	uint32_t ebr_addr;
} mbr_part_t;

/** Partition list structure */
typedef struct mbr_parts {
	/** Number of primary partitions */
	unsigned char n_primary;
	/** Index to the extended partition in the array */
	link_t *l_extended;
	/** Number of logical partitions */
	unsigned int n_logical;
	/** Logical partition linked list */
	list_t list;
} mbr_partitions_t;

/** Both header and partition list */
typedef struct mbr_label {
	/** MBR header */
	mbr_t *mbr;
	/** Partition list */
	mbr_partitions_t * parts;
	/** Device where the data are from (or for) */
	service_id_t device;
} mbr_label_t;

#define mbr_part_foreach(label, iterator) \
	for (iterator = list_get_instance((label)->parts->list.head.next, mbr_part_t, link); \
	    iterator != list_get_instance(&((label)->parts->list.head), mbr_part_t, link); \
	    iterator = list_get_instance(iterator->link.next, mbr_part_t, link))

extern mbr_label_t *mbr_alloc_label(void);

extern void mbr_set_device(mbr_label_t *, service_id_t);
extern mbr_t *mbr_alloc_mbr(void);
extern int mbr_read_mbr(mbr_label_t *, service_id_t);
extern int mbr_write_mbr(mbr_label_t *, service_id_t);
extern int mbr_is_mbr(mbr_label_t *);

extern int mbr_read_partitions(mbr_label_t *);
extern int mbr_write_partitions(mbr_label_t *, service_id_t);
extern mbr_part_t *mbr_alloc_partition(void);
extern mbr_partitions_t *mbr_alloc_partitions(void);
extern mbr_err_val mbr_add_partition(mbr_label_t *, mbr_part_t *);
extern int mbr_remove_partition(mbr_label_t *, size_t);
extern int mbr_get_flag(mbr_part_t *, mbr_flags_t);
extern void mbr_set_flag(mbr_part_t *, mbr_flags_t, bool);
extern uint32_t mbr_get_next_aligned(uint32_t, unsigned int);
extern list_t *mbr_get_list(mbr_label_t *);
extern mbr_part_t *mbr_get_first_partition(mbr_label_t *);
extern mbr_part_t *mbr_get_next_partition(mbr_label_t *, mbr_part_t *);

extern void mbr_free_label(mbr_label_t *);
extern void mbr_free_mbr(mbr_t *);
extern void mbr_free_partition(mbr_part_t *);
extern void mbr_free_partitions(mbr_partitions_t *);

#endif
