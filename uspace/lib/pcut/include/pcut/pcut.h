/*
 * SPDX-FileCopyrightText: 2012-2014 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 * PCUT: Plain-C unit-testing mini-framework.
 */
#ifndef PCUT_PCUT_H_GUARD
#define PCUT_PCUT_H_GUARD

#include <pcut/asserts.h>
#include <pcut/tests.h>


/** PCUT outcome: test passed. */
#define PCUT_OUTCOME_PASS 0

/** PCUT outcome: test failed. */
#define PCUT_OUTCOME_FAIL 1

/** PCUT outcome: test failed unexpectedly. */
#define PCUT_OUTCOME_INTERNAL_ERROR 2

/** PCUT outcome: invalid invocation of the final program. */
#define PCUT_OUTCOME_BAD_INVOCATION 3


#endif
