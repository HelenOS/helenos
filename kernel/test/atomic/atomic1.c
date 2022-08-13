/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <test.h>
#include <atomic.h>

const char *test_atomic1(void)
{
	atomic_int a;

	atomic_store(&a, 10);
	if (atomic_load(&a) != 10)
		return "Failed atomic_store()/atomic_load()";

	if (atomic_postinc(&a) != 10)
		return "Failed atomic_postinc()";
	if (atomic_load(&a) != 11)
		return "Failed atomic_load() after atomic_postinc()";

	if (atomic_postdec(&a) != 11)
		return "Failed atomic_postdec()";
	if (atomic_load(&a) != 10)
		return "Failed atomic_load() after atomic_postdec()";

	if (atomic_preinc(&a) != 11)
		return "Failed atomic_preinc()";
	if (atomic_load(&a) != 11)
		return "Failed atomic_load() after atomic_preinc()";

	if (atomic_predec(&a) != 10)
		return "Failed atomic_predec()";
	if (atomic_load(&a) != 10)
		return "Failed atomic_load() after atomic_predec()";

	return NULL;
}
