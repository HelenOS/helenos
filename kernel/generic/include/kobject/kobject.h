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

#ifndef KERN_KOBJECT_H_
#define KERN_KOBJECT_H_

#include <typedefs.h>
#include <ipc/ipc.h>
#include <ddi/irq.h>

#define MAX_KERNEL_OBJECTS  64

#define for_each_kobject(task, ko, type) \
	for (int i = 0, l = 1; i < MAX_KERNEL_OBJECTS && l; i++) \
		for (kobject_t *(ko) = kobject_get((task), i, (type)); \
		    (ko) && !(l = 0); (ko) = NULL, l = 1)

#define for_each_kobject_current(ko, type) \
	for_each_kobject(TASK, (ko), (type))

typedef enum {
	KOBJECT_TYPE_INVALID,
	KOBJECT_TYPE_ALLOCATED,
	KOBJECT_TYPE_PHONE,
	KOBJECT_TYPE_IRQ
} kobject_type_t;

typedef struct kobject {
	kobject_type_t type;
	bool (* can_reclaim)(struct kobject *);

	union {
		phone_t phone;
		irq_t irq;
	};
} kobject_t;

struct task;

void kobject_task_alloc(struct task *);
void kobject_task_free(struct task *);
void kobject_task_init(struct task *);

extern void kobject_initialize(kobject_t *);
extern kobject_t *kobject_get(struct task *, int, kobject_type_t);
extern kobject_t *kobject_get_current(int, kobject_type_t);
extern int kobject_alloc(struct task *);
extern void kobject_free(struct task *, int);

extern int kobject_to_cap(struct task *, kobject_t *);

#endif

/** @}
 */
