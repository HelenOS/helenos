/*
 * SPDX-FileCopyrightText: 2012 Sean Bartell
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup bithenge
 * @{
 */
/**
 * @file
 * Sequence transforms.
 */

#ifndef BITHENGE_SEQUENCE_H_
#define BITHENGE_SEQUENCE_H_

#include "transform.h"

errno_t bithenge_new_struct(bithenge_transform_t **,
    bithenge_named_transform_t *);
errno_t bithenge_repeat_transform(bithenge_transform_t **, bithenge_transform_t *,
    bithenge_expression_t *);
errno_t bithenge_do_while_transform(bithenge_transform_t **,
    bithenge_transform_t *, bithenge_expression_t *);

#endif

/** @}
 */
