/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 */

#include <stddef.h>
#include "../../../generic/private/thread.h"

void __thread_entry(void)
{
	__thread_main(NULL);
}

/** @}
 */
