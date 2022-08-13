/*
 * SPDX-FileCopyrightText: 2012 Frantisek Princ
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libext4
 * @{
 */
/**
 * @file  hash.c
 * @brief Hashing algorithms for ext4 HTree.
 */

#include <errno.h>
#include "ext4/hash.h"

errno_t ext4_hash_string(ext4_hash_info_t *hinfo, int len, const char *name)
{
	// TODO
	hinfo->hash = 0;
	return ENOTSUP;
}

/**
 * @}
 */
