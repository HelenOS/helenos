/*
 * Copyright (c) 2023 Jiri Svoboda
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

extern loc_srv_t *vbds_srv;

#endif

/** @}
 */
