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

/** @addtogroup kernel_genarch
 * @{
 */
/** @file
 *  @brief DDI.
 */

#include <ddi/ddi.h>
#include <proc/task.h>
#include <typedefs.h>

/** Enable I/O space range for task.
 *
 * Interrupts are disabled and task is locked.
 *
 * @param task   Task.
 * @param ioaddr Starting I/O space address.
 * @param size   Size of the enabled I/O range.
 *
 * @return EOK on success or an error code from errno.h.
 */
errno_t ddi_iospace_enable_arch(task_t *task, uintptr_t ioaddr, size_t size)
{
	return 0;
}

/** Disable I/O space range for task.
 *
 * Interrupts are disabled and task is locked.
 *
 * @param task   Task.
 * @param ioaddr Starting I/O space address.
 * @param size   Size of the disabled I/O range.
 *
 * @return EOK on success or an error code from errno.h.
 */
errno_t ddi_iospace_disable_arch(task_t *task, uintptr_t ioaddr, size_t size)
{
	return 0;
}

/** @}
 */
