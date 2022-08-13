/*
 * SPDX-FileCopyrightText: 2011 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef _LIBC_PRIVATE_THREAD_H_
#define _LIBC_PRIVATE_THREAD_H_

#include <time.h>
#include <abi/proc/uarg.h>
#include <libarch/thread.h>
#include <abi/proc/thread.h>

extern void __thread_entry(void);
extern void __thread_main(uspace_arg_t *);

extern errno_t thread_create(void (*)(void *), void *, const char *,
    thread_id_t *);
extern void thread_exit(int) __attribute__((noreturn));
extern void thread_detach(thread_id_t);
extern thread_id_t thread_get_id(void);
extern void thread_usleep(usec_t);
extern void thread_sleep(sec_t);

#endif

/** @}
 */
