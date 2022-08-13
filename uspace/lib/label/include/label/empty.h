/*
 * SPDX-FileCopyrightText: 2015 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup liblabel
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef LABEL_EMPTY_H_
#define LABEL_EMPTY_H_

#include <loc.h>
#include <types/liblabel.h>

extern errno_t label_bd_is_empty(label_bd_t *, bool *);
extern errno_t label_bd_empty(label_bd_t *);
extern errno_t label_part_empty(label_part_t *);

#endif

/** @}
 */
