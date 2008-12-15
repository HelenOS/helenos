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

uintptr_t ofw_cif;	/**< OpenFirmware Client Interface address. */

phandle ofw_chosen;
ihandle ofw_stdout;
ihandle ofw_mmu;

void ofw_init(void)
{
	ofw_chosen = ofw_find_device("/chosen");
	if (ofw_chosen == -1)
		ofw_done();
	
	if (ofw_get_property(ofw_chosen, "stdout",  &ofw_stdout, sizeof(ofw_stdout)) <= 0)
		ofw_stdout = 0;	

	if (ofw_get_property(ofw_chosen, "mmu",  &ofw_mmu, sizeof(ofw_mmu)) <= 0)
		ofw_mmu = 0;
}

void ofw_done(void)
{
	(void) ofw_call("exit", 0, 1, NULL);
	cpu_halt();
}

/** Perform a call to OpenFirmware client interface.
 *
 * @param service String identifying the service requested.
 * @param nargs Number of input arguments.
 * @param nret Number of output arguments. This includes the return value.
 * @param rets Buffer for output arguments or NULL. The buffer must accomodate nret - 1 items.
 *
 * @return Return value returned by the client interface.
 */
ofw_arg_t ofw_call(const char *service, const int nargs, const int nret, ofw_arg_t *rets, ...)
{
	va_list list;
	ofw_args_t args;
	int i;
	
	args.service = service;
	args.nargs = nargs;
	args.nret = nret;
	
	va_start(list, rets);
	for (i = 0; i < nargs; i++)
		args.args[i] = va_arg(list, ofw_arg_t);
	va_end(list);
	
	for (i = 0; i < nret; i++)
		args.args[i + nargs] = 0;
	
	(void) ofw(&args);

	for (i = 1; i < nret; i++)
		rets[i - 1] = args.args[i + nargs];

	return args.args[nargs];
}

void ofw_putchar(const char ch)
{
	if (ofw_stdout == 0)
		return;
	
	(void) ofw_call("write", 3, 1, NULL, ofw_stdout, &ch, 1);
}

phandle ofw_find_device(const char *name)
{
	return (phandle) ofw_call("finddevice", 1, 1, NULL, name);
}

int ofw_get_property(const phandle device, const char *name, const void *buf, const int buflen)
{
	return (int) ofw_call("getprop", 4, 1, NULL, device, name, buf, buflen);
}

/** Translate virtual address to physical address using OpenFirmware.
 *
 * Use this function only when OpenFirmware is in charge.
 *
 * @param virt Virtual address.
 * @return NULL on failure or physical address on success.
 */
void *ofw_translate(const void *virt)
{
	ofw_arg_t result[4];
	int shift;
	
	if (!ofw_mmu)
		return NULL;
	
	if (ofw_call("call-method", 3, 5, result, "translate", ofw_mmu, virt) != 0)
		return NULL;

	if (result[0] != -1)
		return NULL;
								
	if (sizeof(unative_t) == 8)
		shift = 32;
	else
		shift = 0;
	
	return (void *) ((result[2]<<shift)|result[3]);
}

void *ofw_claim(const void *addr, const int size, const int align)
{
	return (void *) ofw_call("claim", 3, 1, NULL, addr, size, align);
}

/** @}
 */
