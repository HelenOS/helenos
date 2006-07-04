/*
 * Copyright (C) 2005 Martin Decky
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

/** @addtogroup genarch	
 * @{
 */
/** @file
 */

#include <genarch/ofw/ofw.h>
#include <arch/asm.h>
#include <stdarg.h>
#include <cpu.h>
#include <arch/types.h>

ofw_entry ofw;

phandle ofw_chosen;
ihandle ofw_stdin;
ihandle ofw_stdout;

void ofw_init(void)
{
	ofw_chosen = ofw_find_device("/chosen");
	if (ofw_chosen == -1)
		ofw_done();
	
	if (ofw_get_property(ofw_chosen, "stdin",  &ofw_stdin, sizeof(ofw_stdin)) <= 0)
		ofw_stdin = 0;
		
	if (ofw_get_property(ofw_chosen, "stdout",  &ofw_stdout, sizeof(ofw_stdout)) <= 0)
		ofw_stdout = 0;	
}

void ofw_done(void)
{
	(void) ofw_call("exit", 0, 0);
	cpu_halt();
}

unative_t ofw_call(const char *service, const int nargs, const int nret, ...)
{
	va_list list;
	ofw_args_t args;
	int i;
	
	args.service = service;
	args.nargs = nargs;
	args.nret = nret;
	
	va_start(list, nret);
	for (i = 0; i < nargs; i++)
		args.args[i] = va_arg(list, ofw_arg_t);
	va_end(list);
	
	for (i = 0; i < nret; i++)
		args.args[i + nargs] = 0;
	
	ofw(&args);
	
	return args.args[nargs];
}

void ofw_putchar(const char ch)
{
	if (ofw_stdout == 0)
		return;
	
	(void) ofw_call("write", 3, 1, ofw_stdout, &ch, 1);
}

/** Read character from OFW's input.
 *
 * This call is non-blocking.
 *
 * @return 0 if no character was read, character read otherwise.
 */
char ofw_getchar(void)
{
	char ch;

	if (ofw_stdin == 0)
		return 0;
	
	if (ofw_call("read", 3, 1, ofw_stdin, &ch, 1) == 1)
		return ch;
	else
		return 0;
}

phandle ofw_find_device(const char *name)
{
	return (phandle) ofw_call("finddevice", 1, 1, name);
}

int ofw_get_property(const phandle device, const char *name, void *buf, const int buflen)
{
	return (int) ofw_call("getprop", 4, 1, device, name, buf, buflen);
}

void *ofw_claim(const void *addr, const int size, const int align)
{
	return (void *) ofw_call("claim", 3, 1, addr, size, align);
}

/** @}
 */
