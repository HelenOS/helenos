/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
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

#ifndef MKFS_H_
#define MKFS_H_

#include <loc.h>
#include <types/vol.h>

extern errno_t volsrv_part_mkfs(service_id_t, vol_fstype_t, const char *);
extern void volsrv_part_get_lsupp(vol_fstype_t, vol_label_supp_t *);

#endif

/** @}
 */
