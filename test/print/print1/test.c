/*
 * Copyright (C) 2005 Josef Cejka
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
#include <print.h>
#include <test.h>

#define BUFFER_SIZE 32

void test(void)
{
	int retval;
	__native nat = 0x12345678u;
	
	char buffer[BUFFER_SIZE];
	
	printf(" Printf test \n");
	
	printf(" text 10.8s %*.*s \n", 5, 3, "text");
	printf(" very long text 10.8s %10.8s \n", "very long text");
	printf(" text 8.10s %8.10s \n", "text");
	printf(" very long text 8.10s %8.10s \n", "very long text");

	printf(" char: c '%c', 3.2c '%3.2c', -3.2c '%-3.2c', 2.3c '%2.3c', -2.3c '%-2.3c' \n",'a', 'b', 'c', 'd', 'e' );
	printf(" int: d '%d', 3.2d '%3.2d', -3.2d '%-3.2d', 2.3d '%2.3d', -2.3d '%-2.3d' \n",1, 1, 1, 1, 1 );
	printf(" -int: d '%d', 3.2d '%3.2d', -3.2d '%-3.2d', 2.3d '%2.3d', -2.3d '%-2.3d' \n",-1, -1, -1, -1, -1 );
	printf(" 0xint: x '%#x', 5.3x '%#5.3x', -5.3x '%#-5.3x', 3.5x '%#3.5x', -3.5x '%#-3.5x' \n",17, 17, 17, 17, 17 );

	printf("'%#llx' 64bit, '%#x' 32bit, '%#hhx' 8bit, '%#hx' 16bit, __native '%#zx'. '%#llx' 64bit and '%s' string.\n", 0x1234567887654321ll, 0x12345678, 0x12, 0x1234, nat, 0x1234567887654321ull, "Lovely string" );
	
	printf(" Print to NULL '%s'\n",NULL);

	retval = snprintf(buffer, BUFFER_SIZE, "Short text without parameters.");
	printf("Result is: '%s', retval = %d\n", buffer, retval);

	retval = snprintf(buffer, BUFFER_SIZE, "Very very very long text without parameters.");
	printf("Result is: '%s', retval = %d\n", buffer, retval);
	
	printf("Print short text to %d char long buffer via snprintf.\n", BUFFER_SIZE);
	retval = snprintf(buffer, BUFFER_SIZE, "Short %s", "text");
	printf("Result is: '%s', retval = %d\n", buffer, retval);
	
	printf("Print long text to %d char long buffer via snprintf.\n", BUFFER_SIZE);
	retval = snprintf(buffer, BUFFER_SIZE, "Very long %s. This text`s length is more than %d. We are interested in the result.", "text" , BUFFER_SIZE);
	printf("Result is: '%s', retval = %d\n", buffer, retval);
	
	return;
}
