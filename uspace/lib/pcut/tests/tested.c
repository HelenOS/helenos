/*
 * SPDX-FileCopyrightText: 2012-2013 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "tested.h"

#define UNUSED(a) ((void)a)

long intpow(int base, int exp) {
	UNUSED(base); UNUSED(exp);
	return 0;
}

int intmin(int a, int b) {
	UNUSED(b);
	return a;
}
