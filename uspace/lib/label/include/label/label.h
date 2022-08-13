/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup liblabel
 * @{
 */
/**
 * @file Disk label library.
 */

#ifndef LIBLABEL_LABEL_H_
#define LIBLABEL_LABEL_H_

#include <types/label.h>
#include <types/liblabel.h>

extern errno_t label_open(label_bd_t *, label_t **);
extern errno_t label_create(label_bd_t *, label_type_t, label_t **);
extern void label_close(label_t *);
extern errno_t label_destroy(label_t *);
extern errno_t label_get_info(label_t *, label_info_t *);

extern label_part_t *label_part_first(label_t *);
extern label_part_t *label_part_next(label_part_t *);
extern void label_part_get_info(label_part_t *, label_part_info_t *);

extern errno_t label_part_create(label_t *, label_part_spec_t *,
    label_part_t **);
extern errno_t label_part_destroy(label_part_t *);
extern void label_pspec_init(label_part_spec_t *);
extern errno_t label_suggest_ptype(label_t *, label_pcnt_t, label_ptype_t *);

#endif

/** @}
 */
