/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup generic	
 * @{
 */

/**
 * @file
 * @brief	Miscellaneous functions.
 */

#include <func.h>
#include <print.h>
#include <cpu.h>
#include <arch/asm.h>
#include <arch.h>
#include <console/kconsole.h>

atomic_t haltstate = {0}; /**< Halt flag */


/** Halt wrapper
 *
 * Set halt flag and halt the CPU.
 *
 */
void halt()
{
#ifdef CONFIG_DEBUG
	bool rundebugger = false;
	
	if (!atomic_get(&haltstate)) {
		atomic_set(&haltstate, 1);
		rundebugger = true;
	}
#else
	atomic_set(&haltstate, 1);
#endif
	
	interrupts_disable();
	
#if (defined(CONFIG_DEBUG)) && (defined(CONFIG_KCONSOLE))
	if ((rundebugger) && (kconsole_check_poll()))
		kconsole("panic", "\nLast resort kernel console ready.\n", false);
#endif
	
	if (CPU)
		printf("cpu%u: halted\n", CPU->id);
	else
		printf("cpu: halted\n");
	
	cpu_halt();
}

/** Convert ascii representation to unative_t
 *
 * Supports 0x for hexa & 0 for octal notation.
 * Does not check for overflows, does not support negative numbers
 *
 * @param text Textual representation of number
 * @return Converted number or 0 if no valid number ofund 
 */
unative_t atoi(const char *text)
{
	int base = 10;
	unative_t result = 0;

	if (text[0] == '0' && text[1] == 'x') {
		base = 16;
		text += 2;
	} else if (text[0] == '0')
		base = 8;

	while (*text) {
		if (base != 16 && \
		    ((*text >= 'A' && *text <= 'F' )
		     || (*text >='a' && *text <='f')))
			break;
		if (base == 8 && *text >='8')
			break;

		if (*text >= '0' && *text <= '9') {
			result *= base;
			result += *text - '0';
		} else if (*text >= 'A' && *text <= 'F') {
			result *= base;
			result += *text - 'A' + 10;
		} else if (*text >= 'a' && *text <= 'f') {
			result *= base;
			result += *text - 'a' + 10;
		} else
			break;
		text++;
	}

	return result;
}

/** @}
 */
