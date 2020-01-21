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

/** @addtogroup libc
 * @{
 */
/** @file
 * @brief Program loader interface.
 */

#ifndef _LIBC_LOADER_H_
#define _LIBC_LOADER_H_

#include <abi/proc/task.h>

/** Forward declararion */
struct loader;
typedef struct loader loader_t;

extern errno_t loader_spawn(const char *);
extern loader_t *loader_connect(errno_t *);
extern errno_t loader_get_task_id(loader_t *, task_id_t *);
extern errno_t loader_set_cwd(loader_t *);
extern errno_t loader_set_program(loader_t *, const char *, int);
extern errno_t loader_set_program_path(loader_t *, const char *);
extern errno_t loader_set_args(loader_t *, const char *const[]);
extern errno_t loader_add_inbox(loader_t *, const char *, int);
extern errno_t loader_load_program(loader_t *);
extern errno_t loader_run(loader_t *);
extern void loader_run_nowait(loader_t *);
extern void loader_abort(loader_t *);

#endif

/**
 * @}
 */
