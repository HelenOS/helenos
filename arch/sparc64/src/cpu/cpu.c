/*
 * Copyright (C) 2005 Jakub Jermar
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

/** @addtogroup sparc64
 * @{
 */
/** @file
 */

#include <cpu.h>
#include <arch.h>
#include <arch/register.h>
#include <arch/asm.h>
#include <print.h>

void cpu_arch_init(void)
{
}

void cpu_identify(void)
{
	CPU->arch.ver.value = ver_read();
}

void cpu_print_report(cpu_t *m)
{
	char *manuf, *impl;

	switch (CPU->arch.ver.manuf) {
	    case MANUF_FUJITSU:
		manuf = "Fujitsu";
		break;
	    case MANUF_ULTRASPARC:
		manuf = "UltraSPARC";
		break;
	    case MANUF_SUN:
	    	manuf = "Sun";
		break;
	    default:
		manuf = "Unknown";
		break;
	}
	
	switch (CPU->arch.ver.impl) {
	    case IMPL_ULTRASPARCI:
		impl = "UltraSPARC I";
		break;
	    case IMPL_ULTRASPARCII:
		impl = "UltraSPARC II";
		break;
	    case IMPL_ULTRASPARCII_I:
		impl = "UltraSPARC IIi";
		break;
	    case IMPL_ULTRASPARCII_E:
		impl = "UltraSPARC IIe";
		break;
	    case IMPL_ULTRASPARCIII:
		impl = "UltraSPARC III";
		break;
	    case IMPL_ULTRASPARCIV_PLUS:
		impl = "UltraSPARC IV+";
		break;
	    case IMPL_SPARC64V:
		impl = "SPARC 64V";
		break;
	    default:
		impl = "Unknown";
		break;
	}

	printf("cpu%d: manuf=%s, impl=%s, mask=%d\n", CPU->id, manuf, impl, CPU->arch.ver.mask);
}

/** @}
 */
