/*
 * SPDX-FileCopyrightText: 2014 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LIBCOMPRESS_GZIP_H_
#define LIBCOMPRESS_GZIP_H_

#include <stdbool.h>
#include <stddef.h>

extern bool gzip_check(const void *, size_t);
extern size_t gzip_size(const void *, size_t);
extern int gzip_expand(const void *, size_t, void *, size_t);

#endif
