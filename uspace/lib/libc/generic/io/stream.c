/*
 * Copyright (c) 2006 Josef Cejka
 * Copyright (c) 2006 Jakub Vana
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <io/io.h>
#include <io/stream.h>
#include <string.h>
#include <malloc.h>
#include <libc.h>
#include <ipc/ipc.h>
#include <ipc/ns.h>
#include <ipc/fb.h>
#include <ipc/services.h>
#include <ipc/console.h>
#include <console.h>
#include <kbd/kbd.h>
#include <unistd.h>
#include <async.h>
#include <sys/types.h>

ssize_t write_stderr(const void *buf, size_t count)
{
	return count;
}

ssize_t read_stdin(void *buf, size_t count)
{
	int cons_phone = console_phone_get();

	if (cons_phone >= 0) {
		kbd_event_t ev;
		int rc;
		size_t i = 0;
	
		while (i < count) {
			do {
				rc = kbd_get_event(&ev);
				if (rc < 0) return -1;
			} while (ev.c == 0 || ev.type == KE_RELEASE);

			((char *) buf)[i++] = ev.c;
		}
		return i;
	} else {
		return -1;
	}
}

ssize_t write_stdout(const void *buf, size_t count)
{
	int cons_phone = console_phone_get();

	if (cons_phone >= 0) {
		int i;

		for (i = 0; i < count; i++)
			console_putchar(((const char *) buf)[i]);

		return count;
	} else
		return __SYSCALL3(SYS_KLOG, 1, (sysarg_t) buf, count);
}

void klog_update(void)
{
	(void) __SYSCALL3(SYS_KLOG, 1, NULL, 0);
}

/** @}
 */
