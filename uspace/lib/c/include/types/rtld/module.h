/*
 * SPDX-FileCopyrightText: 2008 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
