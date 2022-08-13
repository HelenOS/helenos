/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 */

extern void _start(void);
extern void __c_start(void *);

// FIXME: Implement properly.

void _start(void)
{
	__c_start(0);
}

/** @}
 */
