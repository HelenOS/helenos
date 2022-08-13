/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#ifndef KERN_SMP_H_
#define KERN_SMP_H_

#include <synch/waitq.h>

extern waitq_t ap_completion_wq;

#ifdef CONFIG_SMP

extern void smp_init(void);
extern void kmp(void *arg);

#else /* CONFIG_SMP */

#define smp_init()

#endif /* CONFIG_SMP */

#endif /* __SMP_H__ */

/** @}
 */
