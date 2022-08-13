/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef BOOT_BALLOC_H_
#define BOOT_BALLOC_H_

#include <stddef.h>
#include <stdint.h>

typedef struct {
	uintptr_t base;
	size_t size;
} ballocs_t;

extern void balloc_init(ballocs_t *, void *, uintptr_t, size_t);
extern void *balloc(size_t, size_t);
extern void *balloc_rebase(void *);

#endif
