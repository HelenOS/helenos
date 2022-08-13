/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 */

#include <setjmp.h>
#include <stdbool.h>

int __context_save(__context_t *ctx)
{
	return 0;
}

void __context_restore(__context_t *ctx, int val)
{
	while (true)
		;
}

/** @}
 */
