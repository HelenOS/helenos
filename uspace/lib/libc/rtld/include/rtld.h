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

/** @addtogroup generic	
 * @{
 */
/** @file
 */

#ifndef RTLD_H_
#define RTLD_H_

#include <sys/types.h>
#include <adt/list.h>

#include <dynamic.h>
#include <module.h>

/* Define to enable debugging mode. */
#undef RTLD_DEBUG

#ifdef RTLD_DEBUG
	#define DPRINTF(format, ...) printf(format, ##__VA_ARGS__);
#else
	#define DPRINTF(format, ...)
#endif

typedef struct {
	elf_dyn_t *rtld_dynamic;
	module_t rtld;

	module_t *program;

	/** List of all loaded modules including rtld and the program */
	link_t modules_head;

	/** Temporary hack to place each module at different address. */
	uintptr_t next_bias;
} runtime_env_t;

extern runtime_env_t *runtime_env;

extern void rtld_init_static(void);

#endif

/** @}
 */
