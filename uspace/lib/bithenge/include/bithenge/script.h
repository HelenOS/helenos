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
 * Script parsing.
 */

#ifndef BITHENGE_SCRIPT_H_
#define BITHENGE_SCRIPT_H_

#include "transform.h"

errno_t bithenge_parse_script(const char *, bithenge_transform_t **);

#endif

/** @}
 */
