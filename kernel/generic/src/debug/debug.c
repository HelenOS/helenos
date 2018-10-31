/*
 * Copyright (c) 2010 Martin Decky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
