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

#include "version.h"
#include <ipc.h>
#include <ns.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

/*
static void test_mremap(void)
{
	printf("Writing to good memory\n");
	mremap(&_heap, 120000, 0);
	printf("%P\n", ((char *)&_heap));
	printf("%P\n", ((char *)&_heap) + 80000);
	*(((char *)&_heap) + 80000) = 10;
	printf("Making small\n");
	mremap(&_heap, 16000, 0);
	printf("Failing..\n");
	*((&_heap) + 80000) = 10;

	printf("memory done\n");
}
*/
/*
static void test_sbrk(void)
{
	printf("Writing to good memory\n");
	printf("Got: %P\n", sbrk(120000));
	printf("%P\n", ((char *)&_heap));
	printf("%P\n", ((char *)&_heap) + 80000);
	*(((char *)&_heap) + 80000) = 10;
	printf("Making small, got: %P\n",sbrk(-120000));
	printf("Testing access to heap\n");
	_heap = 10;
	printf("Failing..\n");
	*((&_heap) + 80000) = 10;

	printf("memory done\n");
}
*/
/*
static void test_malloc(void)
{
	char *data;

	data = malloc(10);
	printf("Heap: %P, data: %P\n", &_heap, data);
	data[0] = 'a';
	free(data);
}
*/

int main(int argc, char *argv[])
{
	version_print();

	ipc_call_sync_2(PHONE_NS, NS_PING, 2, 0, 0, 0);
	
	return 0;
}
