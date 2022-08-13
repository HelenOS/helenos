/*
 * SPDX-FileCopyrightText: 2007 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_mm
 * @{
 */
/** @file
 */

#ifndef KERN_MM_H_
#define KERN_MM_H_

#define PAGE_CACHEABLE_SHIFT		0
#define PAGE_NOT_CACHEABLE_SHIFT	PAGE_CACHEABLE_SHIFT
#define PAGE_PRESENT_SHIFT		1
#define PAGE_NOT_PRESENT_SHIFT		PAGE_PRESENT_SHIFT
#define PAGE_USER_SHIFT			2
#define PAGE_KERNEL_SHIFT		PAGE_USER_SHIFT
#define PAGE_READ_SHIFT			3
#define PAGE_WRITE_SHIFT		4
#define PAGE_EXEC_SHIFT			5
#define PAGE_GLOBAL_SHIFT		6

#define PAGE_NOT_CACHEABLE		(0 << PAGE_CACHEABLE_SHIFT)
#define PAGE_CACHEABLE			(1 << PAGE_CACHEABLE_SHIFT)

#define PAGE_PRESENT			(0 << PAGE_PRESENT_SHIFT)
#define PAGE_NOT_PRESENT		(1 << PAGE_PRESENT_SHIFT)

#define PAGE_USER			(1 << PAGE_USER_SHIFT)
#define PAGE_KERNEL			(0 << PAGE_USER_SHIFT)

#define PAGE_READ			(1 << PAGE_READ_SHIFT)
#define PAGE_WRITE			(1 << PAGE_WRITE_SHIFT)
#define PAGE_EXEC			(1 << PAGE_EXEC_SHIFT)

#define PAGE_GLOBAL			(1 << PAGE_GLOBAL_SHIFT)

#endif

/** @}
 */
