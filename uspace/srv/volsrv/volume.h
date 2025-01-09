/*
 * Copyright (c) 2024 Jiri Svoboda
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

#ifndef VOLUME_H_
#define VOLUME_H_

#include "types/vol.h"
#include "types/volume.h"

extern errno_t vol_volumes_create(const char *, vol_volumes_t **);
extern errno_t vol_volumes_merge_to(vol_volumes_t *, const char *);
extern errno_t vol_volumes_sync(vol_volumes_t *);
extern void vol_volumes_destroy(vol_volumes_t *);
extern errno_t vol_volume_lookup_ref(vol_volumes_t *, const char *,
    vol_volume_t **);
extern errno_t vol_volume_find_by_id_ref(vol_volumes_t *, volume_id_t,
    vol_volume_t **);
extern void vol_volume_del_ref(vol_volume_t *);
extern errno_t vol_volume_set_mountp(vol_volume_t *, const char *);
extern errno_t vol_get_ids(vol_volumes_t *, volume_id_t *, size_t,
    size_t *);
extern errno_t vol_volume_get_info(vol_volume_t *, vol_info_t *);

#endif

/** @}
 */
