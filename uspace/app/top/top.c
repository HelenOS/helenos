/*
 * Copyright (c) 2010 Stanislav Kozina
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

/** @addtogroup top
 * @brief Top utility.
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <stdlib.h>
#include <async.h>
#include <unistd.h>
#include <io/console.h>
#include <vfs/vfs.h>
#include "screen.h"
#include "input.h"

int main(int argc, char *argv[])
{
	screen_init();

	/* And paint screen until death... */
	int i = 0;
	while (true) {
		char c = tgetchar();
		if (c < 0) {
			clear_screen();
			moveto(0,0);
			printf("Still running;-) for  %d...", i++);
			fflush(stdout);
			continue;
		}
		switch (c) {
			case 'q':
				return 0;
			default:
				moveto(10,10);
				printf("Unknown command: %c", c);
				fflush(stdout);
				break;
		}
	}

	fflush(stdout);
	return 0;
}

/** @}
 */
