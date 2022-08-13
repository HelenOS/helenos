/*
 * SPDX-FileCopyrightText: 2012 Sean Bartell
 * SPDX-FileCopyrightText: 2012 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BITHENGE_OS_H_
#define BITHENGE_OS_H_

#ifdef __HELENOS__
#include <stdint.h>
typedef int64_t bithenge_int_t;
#define BITHENGE_PRId PRId64

#else
/* Assuming GNU/Linux system. */

#include <inttypes.h>
#include <stdbool.h>
#define BITHENGE_PRId PRIdMAX
typedef intmax_t bithenge_int_t;
typedef uint64_t aoff64_t;
#define EOK 0

#endif

#endif
