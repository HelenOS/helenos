/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic_proc
 * @{
 */
/** @file
 */

#ifndef KERN_PROGRAM_H_
#define KERN_PROGRAM_H_

#include <typedefs.h>

struct task;
struct thread;

/** Program info structure.
 *
 * A program is an abstraction of a freshly created (not yet running)
 * userspace task containing a main thread along with its userspace stack.
 *
 */
typedef struct program {
	struct task *task;           /**< Program task */
	struct thread *main_thread;  /**< Program main thread */
	errno_t loader_status;  /**< Binary loader error status */
} program_t;

extern void *program_loader;

extern errno_t program_create(as_t *, uspace_addr_t, char *, program_t *);
extern errno_t program_create_from_image(void *, char *, program_t *);
extern errno_t program_create_loader(program_t *, char *);
extern void program_ready(program_t *);

extern sys_errno_t sys_program_spawn_loader(uspace_ptr_char, size_t);

#endif

/** @}
 */
