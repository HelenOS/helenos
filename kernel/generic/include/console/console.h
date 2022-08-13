/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

extern char32_t getc(indev_t *indev);
extern size_t gets(indev_t *indev, char *buf, size_t buflen);
extern sys_errno_t sys_kio(int cmd, uspace_addr_t buf, size_t size);

extern void grab_console(void);
extern void release_console(void);

extern sysarg_t sys_debug_console(void);

#endif /* KERN_CONSOLE_H_ */

/** @}
 */
