/*
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup kernel_genarch_mm
 * @{
 */
/**
 * @file
 * @brief This is the generic page hash table interface.
 */

#ifdef CONFIG_PAGE_HT

#ifndef KERN_PAGE_HT_H_
#define KERN_PAGE_HT_H_

#include <typedefs.h>
#include <mm/as.h>
#include <mm/page.h>
#include <mm/slab.h>
#include <adt/hash_table.h>

#define KEY_AS        0
#define KEY_PAGE      1

/* Macros for querying page hash table PTEs. */
#define PTE_VALID(pte)       ((void *) (pte) != NULL)
#define PTE_PRESENT(pte)     ((pte)->p != 0)
#define PTE_GET_FRAME(pte)   ((pte)->frame)
#define PTE_READABLE(pte)    1
#define PTE_WRITABLE(pte)    ((pte)->w != 0)
#define PTE_EXECUTABLE(pte)  ((pte)->x != 0)

extern const as_operations_t as_ht_operations;
extern const page_mapping_operations_t ht_mapping_operations;

extern slab_cache_t *pte_cache;
extern hash_table_t page_ht;
extern const hash_table_ops_t ht_ops;

#endif

#endif

/** @}
 */
