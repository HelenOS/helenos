/*
 * SPDX-FileCopyrightText: 2014 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup sysinst
 * @{
 */
/**
 * @file
 * @brief
 */

#ifndef FUTIL_H
#define FUTIL_H

#include <ipc/loc.h>
#include <stddef.h>

extern errno_t futil_copy_file(const char *, const char *);
extern errno_t futil_rcopy_contents(const char *, const char *);
extern errno_t futil_get_file(const char *, void **, size_t *);

#endif

/** @}
 */
