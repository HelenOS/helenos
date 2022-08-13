/*
 * SPDX-FileCopyrightText: 2015 Petr Pavlu
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm64_proc
 * @{
 */
/** @file
 * @brief Thread related declarations.
 */

#ifndef KERN_arm64_THREAD_H_
#define KERN_arm64_THREAD_H_

typedef struct {
} thread_arch_t;

#define thr_constructor_arch(t)
#define thr_destructor_arch(t)
#define thread_create_arch(t, flags) (EOK)

#endif

/** @}
 */
