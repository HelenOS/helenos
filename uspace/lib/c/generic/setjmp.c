/*
 * SPDX-FileCopyrightText: 2013 Vojtech Horky
 * SPDX-FileCopyrightText: 2018 CZ.NIC, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */

#include <setjmp.h>
#include <context.h>

/** Standard function implementation. */
void longjmp(jmp_buf env, int val)
{
	/*
	 * The standard requires that longjmp() ensures that val is not zero.
	 * __context_restore doesn't do that, so we do it here.
	 */
	__context_restore(env, val == 0 ? 1 : val);
}

/** @}
 */
