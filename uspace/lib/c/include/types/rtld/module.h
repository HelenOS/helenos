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
 */

#ifndef _LIBC_TYPES_RTLD_MODULE_H_
#define _LIBC_TYPES_RTLD_MODULE_H_

#include <adt/list.h>
#include <stddef.h>

typedef enum {
	/** Do not export symbols to global namespace */
	mlf_local = 0x1
} mlflags_t;

/** Dynamically linked module */
typedef struct module {
	/** Module ID */
	unsigned long id;
	/** Dynamic info for this module */
	dyn_info_t dyn;
	/** Load bias */
	size_t bias;

	/** tdata image start */
	void *tdata;
	/** tdata image size */
	size_t tdata_size;
	/** tbss size */
	size_t tbss_size;
	/** TLS alignment */
	size_t tls_align;

	/** Offset of this module's TLS from the thread pointer. */
	ptrdiff_t tpoff;

	/** Containing rtld */
	struct rtld *rtld;
	/** Array of pointers to directly dependent modules */
	struct module **deps;
	/** Number of fields in deps */
	size_t n_deps;

	/** True iff relocations have already been processed in this module. */
	bool relocated;

	/** Link to list of all modules in runtime environment */
	link_t modules_link;
	/** Link to list of initial modules */
	link_t imodules_link;

	/** Link to BFS queue. Only used when doing a BFS of the module graph */
	link_t queue_link;
	/** Tag for modules already processed during a BFS */
	bool bfs_tag;
	/** If @c true, does not export symbols to global namespace */
	bool local;
	/** This is the dynamically linked executable */
	bool exec;
} module_t;

#endif

/** @}
 */
