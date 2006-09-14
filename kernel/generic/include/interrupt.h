/*
 * Copyright (C) 2005 Ondrej Palkovsky
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

/** @addtogroup genericinterrupt
 * @{
 */
/** @file
 */

#ifndef KERN_INTERRUPT_H_
#define KERN_INTERRUPT_H_

#include <arch/interrupt.h>
#include <typedefs.h>
#include <arch/types.h>
#include <proc/task.h>
#include <proc/thread.h>
#include <arch.h>
#include <console/klog.h>
#include <ipc/irq.h>

#ifndef IVT_ITEMS
#	define IVT_ITEMS 0
#endif

#ifndef IVT_FIRST
#	define IVT_FIRST 0
#endif

#define fault_if_from_uspace(istate, cmd, ...) \
{ \
	if (istate_from_uspace(istate)) { \
		klog_printf("Task %lld killed due to an exception at %p.", TASK->taskid, istate_get_pc(istate)); \
		klog_printf("  " cmd, ##__VA_ARGS__); \
		task_kill(TASK->taskid); \
		thread_exit(); \
	} \
}


extern iroutine exc_register(int n, const char *name, iroutine f);
extern void exc_dispatch(int n, istate_t *t);
void exc_init(void);

#endif

/** @}
 */
