/*
 * Copyright (c) 2006 Ondrej Palkovsky
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

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include "../tester.h"

char * test_answer(bool quiet)
{
	int i,cnt, errn = 0;
	char c;

	cnt = 0;
	for (i = 0;i < 50; i++) {
		if (callids[i]) {
			printf("%d: %P\n", cnt, callids[i]);
			cnt++;
		}
		if (cnt >= 10)
			break;
	}
	if (!cnt)
		return NULL;
	printf("Choose message:\n");
	do {
		c = getchar();
	} while (c < '0' || (c-'0') >= cnt);
	cnt = c - '0' + 1;
	
	for (i = 0; cnt; i++)
		if (callids[i])
			cnt--;
	i -= 1;

	printf("Normal (n) or hangup (h) or error(e) message?\n");
	do {
		c = getchar();
	} while (c != 'n' && c != 'h' && c != 'e');
	if (c == 'n')
		errn = 0;
	else if (c == 'h')
		errn = EHANGUP;
	else if (c == 'e')
		errn = ENOENT;
	printf("Answering %P\n", callids[i]);
	ipc_answer_0(callids[i], errn);
	callids[i] = 0;
	
	return NULL;
}
