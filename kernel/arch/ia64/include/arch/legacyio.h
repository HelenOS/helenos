/*
 * SPDX-FileCopyrightText: 2011 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_LEGACYIO_H_
#define KERN_ia64_LEGACYIO_H_

#include <typedefs.h>

#define LEGACYIO_PHYS_BASE	0x00000FFFFC000000ULL

/* Legacy I/O space - static uspace address, FIXME */
#define LEGACYIO_USER_BASE	0x0001000000000000ULL

#define LEGACYIO_PAGE_WIDTH		26	/* 64M */
#define LEGACYIO_SINGLE_PAGE_WIDTH 	12 	/* 4K */

#define LEGACYIO_SIZE	(1ULL << LEGACYIO_PAGE_WIDTH)

extern uintptr_t legacyio_virt_base;

#endif

/** @}
 */
