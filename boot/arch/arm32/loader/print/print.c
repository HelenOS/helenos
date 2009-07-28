/*
 * Copyright (c) 2007 Michal Kebrt
 * Copyright (c) 2009 Vineeth Pillai
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


/** @addtogroup arm32boot
 * @{
 */
/** @file
 *  @brief bootloader output logic
 */ 


#include <printf.h>


/** Address where characters to be printed are expected. */
#ifdef MACHINE_GXEMUL_TESTARM
#define PUTC_ADDRESS	0x10000000
#endif
#ifdef MACHINE_ICP
#define  PUTC_ADDRESS    0x16000000
#endif



/** Prints a character to the console.
 *
 * @param ch Character to be printed.
 */
static void putc(char ch)
{
	if (ch == '\n')
		*((volatile char *) PUTC_ADDRESS) = '\r';
	*((volatile char *) PUTC_ADDRESS) = ch;
}


/** Prints a string to the console.
 *
 * @param str String to be printed.
 * @param len Number of characters to be printed.
 */
void write(const char *str, const int len)
{
	int i;
	for (i = 0; i < len; ++i) {
		putc(str[i]);
	}
}

/** @}
 */

