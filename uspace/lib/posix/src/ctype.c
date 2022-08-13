/*
 * SPDX-FileCopyrightText: 2011 Jiri Zarevucky
 * SPDX-FileCopyrightText: 2011 Petr Koupy
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file Character classification.
 */

#include <ctype.h>

/**
 * Checks whether character is ASCII. (obsolete)
 *
 * @param c Character to inspect.
 * @return Non-zero if character match the definition, zero otherwise.
 */
int isascii(int c)
{
	return c >= 0 && c < 128;
}

/**
 * Converts argument to a 7-bit ASCII character. (obsolete)
 *
 * @param c Character to convert.
 * @return Coverted character.
 */
int toascii(int c)
{
	return c & 0x7F;
}

/** @}
 */
