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

/** @addtogroup volsrv
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef PART_H_
#define PART_H_

#include <loc.h>
#include <stddef.h>
#include <types/vol.h>
#include "types/part.h"
#include "types/volume.h"

extern errno_t vol_parts_create(vol_volumes_t *, vol_parts_t **);
extern void vol_parts_destroy(vol_parts_t *);
extern errno_t vol_part_discovery_start(vol_parts_t *);
extern errno_t vol_part_add_part(vol_parts_t *, service_id_t);
extern errno_t vol_part_get_ids(vol_parts_t *, service_id_t *, size_t,
    size_t *);
extern errno_t vol_part_find_by_id_ref(vol_parts_t *, service_id_t,
    vol_part_t **);
extern errno_t vol_part_find_by_path_ref(vol_parts_t *, const char *,
    vol_part_t **);
extern void vol_part_del_ref(vol_part_t *);
extern errno_t vol_part_eject_part(vol_part_t *);
extern errno_t vol_part_empty_part(vol_part_t *);
extern errno_t vol_part_insert_part(vol_part_t *);
extern errno_t vol_part_mkfs_part(vol_part_t *, vol_fstype_t, const char *,
    const char *);
extern errno_t vol_part_set_mountp_part(vol_part_t *, const char *);
extern errno_t vol_part_get_info(vol_part_t *, vol_part_info_t *);

#endif

/** @}
 */
