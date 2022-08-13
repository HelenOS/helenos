/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file Boxed primitive types binding. */

#include "../builtin.h"
#include "../mytypes.h"

#include "bi_boxed.h"

/** No declaration function. Boxed types are declared in the library. */

/** Bind boxed types.
 *
 * @param bi	Builtin object
 */
void bi_boxed_bind(builtin_t *bi)
{
	bi->boxed_bool = builtin_find_lvl0(bi, "Bool");
	bi->boxed_char = builtin_find_lvl0(bi, "Char");
	bi->boxed_int = builtin_find_lvl0(bi, "Int");
	bi->boxed_string = builtin_find_lvl0(bi, "String");
}
