/*
 * Copyright (c) 2001-2004 Jakub Jermar
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

/** @addtogroup genericproc
 * @{
 */
/** @file
 */

#ifndef KERN_THREAD_H_
#define KERN_THREAD_H_

#include <synch/waitq.h>
#include <proc/task.h>
#include <cpu.h>
#include <synch/rwlock.h>
#include <adt/btree.h>
#include <mm/slab.h>
#include <arch/cpu.h>
#include <mm/tlb.h>
#include <proc/uarg.h>

#define THREAD_STACK_SIZE	STACK_SIZE

extern char *thread_states[];

/* Thread flags */
#define THREAD_FLAG_WIRED	(1 << 0)	/**< Thread cannot be migrated to another CPU. */
#define THREAD_FLAG_STOLEN	(1 << 1)	/**< Thread was migrated to another CPU and has not run yet. */
#define THREAD_FLAG_USPACE	(1 << 2)	/**< Thread executes in userspace. */

/** Thread list lock.
 *
 * This lock protects all link_t structures chained in threads_head.
 * Must be acquired before T.lock for each T of type thread_t.
 *
 */
extern spinlock_t threads_lock;

extern btree_t threads_btree;			/**< B+tree containing all threads. */

extern void thread_init(void);
extern thread_t *thread_create(void (* func)(void *), void *arg, task_t *task, int flags, char *name, bool uncounted);
extern void thread_ready(thread_t *t);
extern void thread_exit(void) __attribute__((noreturn));

#ifndef thread_create_arch
extern void thread_create_arch(thread_t *t);
#endif
#ifndef thr_constructor_arch
extern void thr_constructor_arch(thread_t *t);
#endif
#ifndef thr_destructor_arch
extern void thr_destructor_arch(thread_t *t);
#endif

extern void thread_sleep(uint32_t sec);
extern void thread_usleep(uint32_t usec);

#define thread_join(t)	thread_join_timeout((t), SYNCH_NO_TIMEOUT, SYNCH_FLAGS_NONE)
extern int thread_join_timeout(thread_t *t, uint32_t usec, int flags);
extern void thread_detach(thread_t *t);

extern void thread_register_call_me(void (* call_me)(void *), void *call_me_with);
extern void thread_print_list(void);
extern void thread_destroy(thread_t *t);
extern void thread_update_accounting(void);
extern bool thread_exists(thread_t *t);
extern void thread_interrupt_sleep(thread_t *t);

/* Fpu context slab cache */
extern slab_cache_t *fpu_context_slab;

/** Thread syscall prototypes. */
unative_t sys_thread_create(uspace_arg_t *uspace_uarg, char *uspace_name);
unative_t sys_thread_exit(int uspace_status);

#endif

/** @}
 */
