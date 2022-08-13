/*
 * SPDX-FileCopyrightText: 2012 Frantisek Princ
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libext4
 * @{
 */

#ifndef LIBEXT4_HASH_H_
#define LIBEXT4_HASH_H_

#include "ext4/types.h"

extern errno_t ext4_hash_string(ext4_hash_info_t *, int, const char *);

#endif

/**
 * @}
 */
