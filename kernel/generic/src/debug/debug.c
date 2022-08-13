/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_debug
 * @{
 */

/**
 * @file
 * @brief Kernel instrumentation functions.
 */

#ifdef CONFIG_TRACE

#include <debug.h>
#include <symtab.h>
#include <errno.h>
#include <stdio.h>

void __cyg_profile_func_enter(void *fn, void *call_site)
{
	const char *fn_sym = symtab_fmt_name_lookup((uintptr_t) fn);

	const char *call_site_sym;
	uintptr_t call_site_off;

	if (symtab_name_lookup((uintptr_t) call_site, &call_site_sym,
	    &call_site_off) == EOK)
		printf("%s()+%p->%s()\n", call_site_sym,
		    (void *) call_site_off, fn_sym);
	else
		printf("->%s()\n", fn_sym);
}

void __cyg_profile_func_exit(void *fn, void *call_site)
{
	const char *fn_sym = symtab_fmt_name_lookup((uintptr_t) fn);

	const char *call_site_sym;
	uintptr_t call_site_off;

	if (symtab_name_lookup((uintptr_t) call_site, &call_site_sym,
	    &call_site_off) == EOK)
		printf("%s()+%p<-%s()\n", call_site_sym,
		    (void *) call_site_off, fn_sym);
	else
		printf("<-%s()\n", fn_sym);
}

#endif /* CONFIG_TRACE */

/** @}
 */
