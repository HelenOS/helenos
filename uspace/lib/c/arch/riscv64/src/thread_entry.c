/*
 * SPDX-FileCopyrightText: 2016 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 */

#include <types/common.h>
#include "../../../generic/private/thread.h"

void __thread_entry(void)
{
	__thread_main((void *) 0);
}

/** @}
 */
