/*
 * SPDX-FileCopyrightText: 2014 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 * Helper macros.
 */
#ifndef PCUT_HELPER_H_GUARD
#define PCUT_HELPER_H_GUARD

/** @cond devel */

/** Join the two arguments on preprocessor level (inner call). */
#define PCUT_JOIN_IMPL(a, b) a##b

/** Join the two arguments on preprocessor level. */
#define PCUT_JOIN(a, b) PCUT_JOIN_IMPL(a, b)

/** Quote the parameter (inner call). */
#define PCUT_QUOTE_IMPL(x) #x

/** Quote the parameter. */
#define PCUT_QUOTE(x) PCUT_QUOTE_IMPL(x)

/** Get first argument from macro var-args. */
#define PCUT_VARG_GET_FIRST(x, ...) x

/** Get all but first arguments from macro var-args. */
#define PCUT_VARG_SKIP_FIRST(x, ...) __VA_ARGS__

#endif
