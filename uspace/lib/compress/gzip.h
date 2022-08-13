/*
 * SPDX-FileCopyrightText: 2014 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCOMPRESS_GZIP_H_
#define LIBCOMPRESS_GZIP_H_

#include <stddef.h>

extern errno_t gzip_expand(void *, size_t, void **, size_t *);

#endif
