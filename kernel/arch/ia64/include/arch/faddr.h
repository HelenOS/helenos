/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ia64
 * @{
 */
/** @file
 */

#ifndef KERN_ia64_FADDR_H_
#define KERN_ia64_FADDR_H_

#include <typedefs.h>

/**
 *
 * Calculate absolute address of function
 * referenced by fptr pointer.
 *
 * @param fptr Function pointer.
 *
 */
#define FADDR(fptr)  (((fncptr_t *) (fptr))->fnc)

#endif

/** @}
 */
