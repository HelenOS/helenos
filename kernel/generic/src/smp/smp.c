/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */

/**
 * @file
 */

#include <smp/smp.h>

#ifdef CONFIG_SMP

waitq_t ap_completion_wq;

#endif /* CONFIG_SMP */

/** @}
 */
