/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#include <proc/thread.h>
#include <proc/task.h>
#include <mm/vm.h>

#include <synch/spinlock.h>
#include <arch.h>
#include <panic.h>
#include <list.h>

spinlock_t tasks_lock;
link_t tasks_head;

void task_init(void)
{
	the->task = NULL;
	spinlock_initialize(&tasks_lock);
	list_initialize(&tasks_head);
}

task_t *task_create(vm_t *m)
{
	pri_t pri;
	task_t *ta;
	
	ta = (task_t *) malloc(sizeof(task_t));
	if (ta) {
		spinlock_initialize(&ta->lock);
		list_initialize(&ta->th_head);
		list_initialize(&ta->tasks_link);
		ta->vm = m;
		
		pri = cpu_priority_high();
		spinlock_lock(&tasks_lock);
		list_append(&ta->tasks_link, &tasks_head);
		spinlock_unlock(&tasks_lock);
		cpu_priority_restore(pri);
	}
	return ta;
}

