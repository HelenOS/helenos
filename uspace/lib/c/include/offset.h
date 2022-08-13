/*
 * SPDX-FileCopyrightText: 2017 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_OFFSET_H_
#define _LIBC_OFFSET_H_

#ifndef _HELENOS_SOURCE
#error This file should only be included from HelenOS sources
#endif

#include <stdint.h>
#include <_bits/decls.h>
#include <_bits/off64_t.h>

/* off64_t */
#define OFF64_MIN  INT64_MIN
#define OFF64_MAX  INT64_MAX

/* aoff64_t */
#define AOFF64_MIN  UINT64_MIN
#define AOFF64_MAX  UINT64_MAX

/* off64_t, aoff64_t */
#define PRIdOFF64 PRId64
#define PRIuOFF64 PRIu64
#define PRIxOFF64 PRIx64
#define PRIXOFF64 PRIX64

__HELENOS_DECLS_BEGIN;

/** Absolute offset */
typedef uint64_t aoff64_t;

__HELENOS_DECLS_END;

#endif

/** @}
 */
