/*
 * Copyright (c) 2005 Jakub Jermar
 * Copyright (c) 2011 Jiri Svoboda
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

/** @addtogroup kbd_port
 * @ingroup  kbd
 * @{
 */
/** @file
 * @brief Ski console keyboard port driver.
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <thread.h>
#include <stdbool.h>
#include "../kbd_port.h"
#include "../kbd.h"

static int ski_port_init(kbd_dev_t *);
static void ski_port_write(uint8_t data);

kbd_port_ops_t ski_port = {
	.init = ski_port_init,
	.write = ski_port_write
};

static kbd_dev_t *kbd_dev;

#define SKI_GETCHAR		21

#define POLL_INTERVAL		10000

static void ski_thread_impl(void *arg);
static int32_t ski_getchar(void);

/** Initialize Ski port driver. */
static int ski_port_init(kbd_dev_t *kdev)
{
	thread_id_t tid;
	int rc;

	kbd_dev = kdev;

	rc = thread_create(ski_thread_impl, NULL, "kbd_poll", &tid);
	if (rc != 0) {
		return rc;
	}

	return 0;
}

static void ski_port_write(uint8_t data)
{
	(void) data;
}

/** Thread to poll Ski for keypresses. */
static void ski_thread_impl(void *arg)
{
	int32_t c;
	(void) arg;

	while (1) {
		while (1) {
			c = ski_getchar();
			if (c == 0)
				break;
			
			kbd_push_data(kbd_dev, c);
		}

		usleep(POLL_INTERVAL);
	}
}

/** Ask Ski if a key was pressed.
 *
 * Use SSC (Simulator System Call) to get character from the debug console.
 * This call is non-blocking.
 *
 * @return ASCII code of pressed key or 0 if no key pressed.
 */
static int32_t ski_getchar(void)
{
	uint64_t ch;
	
#ifdef UARCH_ia64
	asm volatile (
		"mov r15 = %1\n"
		"break 0x80000;;\n"	/* modifies r8 */
		"mov %0 = r8;;\n"		

		: "=r" (ch)
		: "i" (SKI_GETCHAR)
		: "r15", "r8"
	);
#else
	ch = 0;
#endif
	return (int32_t) ch;
}

/** @}
 */
