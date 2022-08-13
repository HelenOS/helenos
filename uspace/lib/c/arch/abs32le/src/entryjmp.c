/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 */

#include <stdbool.h>
#include <entry_point.h>

/** Jump to program entry point. */
void entry_point_jmp(void *entry_point, void *pcb)
{
	while (true)
		;
}

/**
 * @}
 */
