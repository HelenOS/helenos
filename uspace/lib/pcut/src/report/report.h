/*
 * SPDX-FileCopyrightText: 2012-2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 *
 * Test results reporting.
 */

#ifndef PCUT_REPORT_H_GUARD
#define PCUT_REPORT_H_GUARD

#include "../internal.h"

/** Reporting functions for the test-anything-protocol. */
extern pcut_report_ops_t pcut_report_tap;

/** Reporting functions for XML report output. */
extern pcut_report_ops_t pcut_report_xml;

#endif
