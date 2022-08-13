/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup vbd
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef DISK_H_
#define DISK_H_

#include <loc.h>
#include "types/vbd.h"
#include <vbd.h>

extern errno_t vbds_disks_init(void);
extern errno_t vbds_disk_discovery_start(void);
extern errno_t vbds_disk_add(service_id_t);
extern errno_t vbds_disk_remove(service_id_t);
extern errno_t vbds_disk_get_ids(service_id_t *, size_t, size_t *);
extern errno_t vbds_disk_info(service_id_t, vbd_disk_info_t *);
extern errno_t vbds_get_parts(service_id_t, service_id_t *, size_t, size_t *);
extern errno_t vbds_label_create(service_id_t, label_type_t);
extern errno_t vbds_label_delete(service_id_t);
extern errno_t vbds_part_get_info(vbds_part_id_t, vbd_part_info_t *);
extern errno_t vbds_part_create(service_id_t, vbd_part_spec_t *, vbds_part_id_t *);
extern errno_t vbds_part_delete(vbds_part_id_t);
extern errno_t vbds_suggest_ptype(service_id_t, label_pcnt_t, label_ptype_t *);
extern void vbds_bd_conn(ipc_call_t *, void *);

#endif

/** @}
 */
