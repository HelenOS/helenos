/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libfdisk
 * @{
 */
/**
 * @file Disk management library.
 */

#ifndef LIBFDISK_FDISK_H_
#define LIBFDISK_FDISK_H_

#include <errno.h>
#include <loc.h>
#include <types/fdisk.h>
#include <types/vol.h>

extern errno_t fdisk_create(fdisk_t **);
extern void fdisk_destroy(fdisk_t *);
extern errno_t fdisk_dev_list_get(fdisk_t *, fdisk_dev_list_t **);
extern void fdisk_dev_list_free(fdisk_dev_list_t *);
extern fdisk_dev_info_t *fdisk_dev_first(fdisk_dev_list_t *);
extern fdisk_dev_info_t *fdisk_dev_next(fdisk_dev_info_t *);
extern errno_t fdisk_dev_info_get_svcname(fdisk_dev_info_t *, char **);
extern void fdisk_dev_info_get_svcid(fdisk_dev_info_t *, service_id_t *);
extern errno_t fdisk_dev_info_capacity(fdisk_dev_info_t *, capa_spec_t *);

extern errno_t fdisk_dev_open(fdisk_t *, service_id_t, fdisk_dev_t **);
extern void fdisk_dev_close(fdisk_dev_t *);
extern errno_t fdisk_dev_erase(fdisk_dev_t *);
extern void fdisk_dev_get_flags(fdisk_dev_t *, fdisk_dev_flags_t *);
extern errno_t fdisk_dev_get_svcname(fdisk_dev_t *, char **);
extern errno_t fdisk_dev_capacity(fdisk_dev_t *, capa_spec_t *);

extern errno_t fdisk_label_get_info(fdisk_dev_t *, fdisk_label_info_t *);
extern errno_t fdisk_label_create(fdisk_dev_t *, label_type_t);
extern errno_t fdisk_label_destroy(fdisk_dev_t *);

extern fdisk_part_t *fdisk_part_first(fdisk_dev_t *);
extern fdisk_part_t *fdisk_part_next(fdisk_part_t *);
extern errno_t fdisk_part_get_info(fdisk_part_t *, fdisk_part_info_t *);
extern errno_t fdisk_part_get_max_avail(fdisk_dev_t *, fdisk_spc_t, capa_spec_t *);
extern errno_t fdisk_part_get_tot_avail(fdisk_dev_t *, fdisk_spc_t, capa_spec_t *);
extern errno_t fdisk_part_create(fdisk_dev_t *, fdisk_part_spec_t *,
    fdisk_part_t **);
extern errno_t fdisk_part_destroy(fdisk_part_t *);
extern errno_t fdisk_part_set_mountp(fdisk_part_t *, const char *);
extern void fdisk_pspec_init(fdisk_part_spec_t *);

extern errno_t fdisk_get_vollabel_support(fdisk_dev_t *, vol_fstype_t,
    vol_label_supp_t *);

#endif

/** @}
 */
