/*
 * SPDX-FileCopyrightText: 2011 Martin Sucha
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup tester
 * @{
 */
/**
 * @file
 */

#include "util.h"
#undef ASSERT_EQ_FN_DEF
#define ASSERT_EQ_FN_DEF(type, fmt, fmtx) \
bool assert_eq_fn_##type(type exp, type act, const char *act_desc) \
{ \
	if (exp != act) { \
		fprintf(stderr, "Expected %s to be %" fmt " (0x%" fmtx "),"\
		    " but was %" fmt " (0x%" fmtx ") \n", \
		    act_desc, exp, exp, act, act); \
	} \
	return exp != act; \
}

#include "util_functions.def"

/** @}
 */
