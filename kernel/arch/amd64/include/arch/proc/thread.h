/*
 * SPDX-FileCopyrightText: 2005 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64_proc
 * @{
 */
/** @file
 */

#ifndef KERN_amd64_THREAD_H_
#define KERN_amd64_THREAD_H_

#include <stdint.h>

typedef struct {
	uint64_t kstack_rsp;
} thread_arch_t;

#define thr_constructor_arch(t)
#define thr_destructor_arch(t)

#endif

/** @}
 */
