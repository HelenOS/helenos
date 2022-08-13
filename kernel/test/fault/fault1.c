/*
 * SPDX-FileCopyrightText: 2005 Jakub Vana
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test.h>

const char *test_fault1(void)
{
	((int *)(0))[1] = 0;

	return "Written to NULL";
}
