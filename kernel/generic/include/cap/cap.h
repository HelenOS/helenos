/*
 * Copyright (c) 2017 Jakub Jermar
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

#ifndef KERN_CAP_H_
#define KERN_CAP_H_

#include <typedefs.h>
#include <ipc/ipc.h>

#define MAX_CAPS  64

#define for_each_cap(task, cap, type) \
	for (int i = 0, l = 1; i < MAX_CAPS && l; i++) \
		for (cap_t *(cap) = cap_get((task), i, (type)); \
		    (cap) && !(l = 0); (cap) = NULL, l = 1)

#define for_each_cap_current(cap, type) \
	for_each_cap(TASK, (cap), (type))

typedef enum {
	CAP_TYPE_INVALID,
	CAP_TYPE_ALLOCATED,
	CAP_TYPE_PHONE,
	CAP_TYPE_IRQ
} cap_type_t;

typedef struct cap {
	cap_type_t type;
	int handle;

	bool (* can_reclaim)(struct cap *);

	/* The underlying kernel object. */
	void *kobject;
} cap_t;

struct task;

void caps_task_alloc(struct task *);
void caps_task_free(struct task *);
void caps_task_init(struct task *);

extern void cap_initialize(cap_t *, int);
extern cap_t *cap_get(struct task *, int, cap_type_t);
extern cap_t *cap_get_current(int, cap_type_t);
extern int cap_alloc(struct task *);
extern void cap_free(struct task *, int);

#endif

/** @}
 */
