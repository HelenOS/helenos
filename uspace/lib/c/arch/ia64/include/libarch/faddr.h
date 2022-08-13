/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcia64
 * @{
 */
/** @file
 */

#ifndef _LIBC_ia64_FADDR_H_
#define _LIBC_ia64_FADDR_H_

#include <types/common.h>

typedef struct {
	uintptr_t fnc;
	uintptr_t gp;
} __attribute__((may_alias)) fncptr_t;

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
