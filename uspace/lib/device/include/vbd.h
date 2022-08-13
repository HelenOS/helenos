/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libdevice
 * @{
 */
/** @file
 */

#ifndef LIBDEVICE_VBD_H
#define LIBDEVICE_VBD_H

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
