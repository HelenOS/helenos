/*
 * Copyright (c) 2005 Jakub Jermar
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

/** @addtogroup kernel_generic_console
 * @{
 */
/** @file
 */

#ifndef KERN_CONSOLE_H_
#define KERN_CONSOLE_H_

#include <typedefs.h>
#include <print.h>
#include <console/chardev.h>
#include <synch/spinlock.h>

#define PAGING(counter, increment, before, after) \
	do { \
		(counter) += (increment); \
		if ((counter) > 23) { \
			before; \
			printf(" -- Press any key to continue -- "); \
			indev_pop_character(stdin); \
			after; \
			printf("\n"); \
			(counter) = 0; \
		} \
	} while (0)

extern indev_t *stdin;
extern outdev_t *stdout;

extern void early_putuchar(char32_t);

extern indev_t *stdin_wire(void);
extern void stdout_wire(outdev_t *outdev);
extern void console_init(void);

extern void kio_init(void);
extern void kio_update(void *);
extern void kio_flush(void);
extern void kio_push_char(const char32_t);
SPINLOCK_EXTERN(kio_lock);

extern sys_errno_t sys_kio(int cmd, uspace_addr_t buf, size_t size);

extern void grab_console(void);
extern void release_console(void);

extern sysarg_t sys_debug_console(void);

#endif /* KERN_CONSOLE_H_ */

/** @}
 */
