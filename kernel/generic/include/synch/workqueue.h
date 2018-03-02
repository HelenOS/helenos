/*
 * Copyright (c) 2012 Adam Hraska
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

#ifndef KERN_WORKQUEUE_H_
#define KERN_WORKQUEUE_H_

#include <adt/list.h>

/* Fwd decl. */
struct thread;
struct work_item;
struct work_queue;
typedef struct work_queue work_queue_t;

typedef void (*work_func_t)(struct work_item *);

typedef struct work_item {
	link_t queue_link;
	work_func_t func;

#ifdef CONFIG_DEBUG
	/* Magic number for integrity checks. */
	uint32_t cookie;
#endif
} work_t;



extern void workq_global_init(void);
extern void workq_global_worker_init(void);
extern void workq_global_stop(void);
extern bool workq_global_enqueue_noblock(work_t *, work_func_t);
extern bool workq_global_enqueue(work_t *, work_func_t);

extern struct work_queue *workq_create(const char *);
extern void workq_destroy(struct work_queue *);
extern bool workq_init(struct work_queue *, const char *);
extern void workq_stop(struct work_queue *);
extern bool workq_enqueue_noblock(struct work_queue *, work_t *, work_func_t);
extern bool workq_enqueue(struct work_queue *, work_t *, work_func_t);

extern void workq_print_info(struct work_queue *);
extern void workq_global_print_info(void);


extern void workq_after_thread_ran(void);
extern void workq_before_thread_is_ready(struct thread *);

#endif /* KERN_WORKQUEUE_H_ */

/** @}
 */
