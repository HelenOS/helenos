/*
 * SPDX-FileCopyrightText: 2012 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_ipc
 * @{
 */
/** @file
 */

#ifndef KERN_SYSIPC_PRIV_H_
#define KERN_SYSIPC_PRIV_H_

#include <ipc/ipc.h>

extern errno_t answer_preprocess(call_t *, ipc_data_t *);

#endif

/** @}
 */
