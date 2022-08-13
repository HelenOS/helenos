/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 */

#include <stddef.h>

extern void _start(void);
extern void __c_start(void *);

/* Normally, the entry point is defined in assembly for the architecture. */

void _start(void)
{
	__c_start(NULL);
}

/** @}
 */
