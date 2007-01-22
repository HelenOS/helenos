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

/** @addtogroup generic	
 * @{
 */
/** @file
 */

#ifndef KERN_TYPEDEFS_H_
#define KERN_TYPEDEFS_H_

#define false 0
#define true 1

typedef short bool;

typedef unsigned long size_t;
typedef unsigned long count_t;
typedef unsigned long index_t;

typedef unsigned long long task_id_t;
typedef unsigned long context_id_t;

typedef struct task task_t;
typedef struct thread thread_t;
typedef struct context context_t;
typedef struct fpu_context fpu_context_t;

typedef struct timeout timeout_t;

typedef struct runq runq_t;

typedef struct spinlock spinlock_t;
typedef struct mutex mutex_t;
typedef struct semaphore semaphore_t;
typedef struct rwlock rwlock_t;
typedef enum rwlock_type rwlock_type_t;
typedef struct condvar condvar_t;
typedef struct waitq waitq_t;
typedef struct futex futex_t;

typedef struct buddy_system buddy_system_t;
typedef struct buddy_system_operations buddy_system_operations_t;

typedef struct as_area as_area_t;
typedef struct as as_t;

typedef struct link link_t;

typedef struct the the_t;

typedef struct chardev chardev_t;

typedef enum cmd_arg_type cmd_arg_type_t;
typedef struct cmd_arg cmd_arg_t;
typedef struct cmd_info cmd_info_t;

typedef struct istate istate_t;
typedef void (* function)();
typedef void (* iroutine)(int n, istate_t *istate);

typedef struct hash_table hash_table_t;
typedef struct hash_table_operations hash_table_operations_t;

typedef struct btree_node btree_node_t;
typedef struct btree btree_t;

typedef signed int inr_t;
typedef signed int devno_t;
typedef struct irq irq_t;
typedef struct ipc_notif_cfg ipc_notif_cfg_t;

#endif

/** @}
 */
