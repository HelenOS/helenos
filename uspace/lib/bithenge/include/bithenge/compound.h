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
 * Compound transforms.
 */

#ifndef BITHENGE_COMPOUND_H_
#define BITHENGE_COMPOUND_H_

#include "expression.h"
#include "transform.h"

errno_t bithenge_new_composed_transform(bithenge_transform_t **,
    bithenge_transform_t **, size_t);
errno_t bithenge_if_transform(bithenge_transform_t **, bithenge_expression_t *,
    bithenge_transform_t *, bithenge_transform_t *);
errno_t bithenge_partial_transform(bithenge_transform_t **,
    bithenge_transform_t *);

#endif

/** @}
 */
