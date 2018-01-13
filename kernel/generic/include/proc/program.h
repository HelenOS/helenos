/*
 * Copyright (c) 2008 Jiri Svoboda
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
	unsigned int loader_status;  /**< Binary loader error status */
} program_t;

extern void *program_loader;

extern errno_t program_create(as_t *, uintptr_t, char *, program_t *);
extern errno_t program_create_from_image(void *, char *, program_t *);
extern errno_t program_create_loader(program_t *, char *);
extern void program_ready(program_t *);

extern sys_errno_t sys_program_spawn_loader(char *, size_t);

#endif

/** @}
 */
