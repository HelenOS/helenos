/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_sparc64_proc
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_THREAD_H_
#define KERN_sparc64_THREAD_H_

#include <arch/arch.h>
#include <mm/slab.h>
#include <stdint.h>

extern slab_cache_t *uwb_cache;

typedef struct {
	/** Buffer for register windows with userspace content. */
	uint8_t *uspace_window_buffer;
} thread_arch_t;

#endif

/** @}
 */
