/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_SMC_H_
#define _LIBC_SMC_H_

#include <errno.h>
#include <stddef.h>

extern errno_t smc_coherence(void *address, size_t size);

#endif

/** @}
 */
