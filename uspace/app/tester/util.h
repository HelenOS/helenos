/*
 * SPDX-FileCopyrightText: 2011 Martin Sucha
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup tester
 * @{
 */
/** @file
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <stdio.h>
#include <inttypes.h>
#include "tester.h"

#define ASSERT_EQ_FN_DEF(type, fmt, fmtx) \
extern bool assert_eq_fn_##type(type, type, const char *);

#include "util_functions.def"

#define ASSERT_EQ_64(exp, act, msg) \
{if (assert_eq_fn_uint64_t(exp, act, #act)) {return msg;}}

#define ASSERT_EQ_32(exp, act, msg) \
{if (assert_eq_fn_uint32_t(exp, act, #act)) {return msg;}}

#define ASSERT_EQ_16(exp, act, msg) \
{if (assert_eq_fn_uint16_t(exp, act, #act)) {return msg;}}

#define ASSERT_EQ_8(exp, act, msg) \
{if (assert_eq_fn_uint8_t(exp, act, #act)) {return msg;}}

#endif

/** @}
 */
