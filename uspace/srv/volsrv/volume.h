/*
 * SPDX-FileCopyrightText: 2018 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
