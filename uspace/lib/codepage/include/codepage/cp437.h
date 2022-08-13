/*
 * SPDX-FileCopyrightText: 2021 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcodepage
 * @{
 */
/**
 * @file Code page 437
 */

#ifndef _CODEPAGE_CP437_H
#define _CODEPAGE_CP437_H

#include <errno.h>
#include <stdint.h>
#include <str.h>

extern char32_t cp437_decode(uint8_t);
extern errno_t cp437_encode(char32_t, uint8_t *);

#endif

/** @}
 */
