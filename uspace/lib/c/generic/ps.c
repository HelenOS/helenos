/*
 * Copyright (c) 2010 Stanislav Kozina
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

#include <task.h>
#include <thread.h>
#include <ps.h>
#include <libc.h>

/** Get the list of task ids.
 *
 * @param ids		Pointer to space where list of ids will be stored.
 * @param size		Total size of allocated space under ids.
 *
 * @return		Count of written task ids. If higher than size, there
 * 			was not enough space.
 *
 */
size_t get_task_ids(task_id_t *ids, size_t size)
{
	return __SYSCALL2(SYS_PS_GET_TASKS, (sysarg_t) ids, (sysarg_t) size);
}

/** Get task info.
 *
 * @param id		Id of the task to get info about.
 * @param info		Pointer to out info struct.
 *
 * @return		0 on success.
 *
 */
int get_task_info(task_id_t id, task_info_t *info)
{
	return __SYSCALL2(SYS_PS_GET_TASK_INFO, (sysarg_t) &id, (sysarg_t) info);
}

/** Get thread infos of the selected task.
 *
 * @param taskid 	Id of the selected task.
 * @param infos		Pointer to out array of thread_info_t structures.
 * @param size		Size of the infos array.
 *
 * @return		Count of written thread_infos. If higher than size, there
 * 			was not enough space.
 *
 */
size_t get_task_threads(thread_info_t *infos, size_t size)
{
	return __SYSCALL2(SYS_PS_GET_THREADS, (sysarg_t) infos, (sysarg_t) size);
}

/** @}
 */
