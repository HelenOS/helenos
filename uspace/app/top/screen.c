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
#include <io/console.h>
#include <vfs/vfs.h>
#include <futex.h>
#include "screen.h"

futex_t screen_lock;

static void resume_normal(void)
{
	fflush(stdout);
	console_set_rgb_color(fphone(stdout), 0, 0xf0f0f0);
}

void screen_init(void)
{
	console_cursor_visibility(fphone(stdout), 0);
	resume_normal();
	clear_screen();
}

void clear_screen(void)
{
	console_clear(fphone(stdout));
	moveto(0, 0);
}

void moveto(int r, int c)
{
	fflush(stdout);
	console_goto(fphone(stdout), c, r);
}

/** @}
 */
