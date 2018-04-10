/*
 * Copyright (c) 2001-2004 Jakub Jermar
 * Copyright (c) 2008 Jiri Svoboda
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

/** @addtogroup genericproc
 * @{
 */

/**
 * @file
 * @brief Running userspace programs.
 */

#include <main/uinit.h>
#include <proc/thread.h>
#include <proc/task.h>
#include <mm/as.h>
#include <mm/slab.h>
#include <arch.h>
#include <adt/list.h>
#include <ipc/ipc.h>
#include <ipc/ipcrsc.h>
#include <security/perm.h>
#include <lib/elf_load.h>
#include <errno.h>
#include <log.h>
#include <syscall/copy.h>
#include <proc/program.h>

/**
 * Points to the binary image used as the program loader. All non-initial
 * tasks are created from this executable image.
 */
void *program_loader = NULL;

/** Create a program using an existing address space.
 *
 * @param as         Address space containing a binary program image.
 * @param entry_addr Program entry-point address in program address space.
 * @param name       Name to set for the program's task.
 * @param prg        Buffer for storing program information.
 *
 * @return EOK on success or an error code.
 *
 */
errno_t program_create(as_t *as, uintptr_t entry_addr, char *name, program_t *prg)
{
	prg->loader_status = EE_OK;
	prg->task = task_create(as, name);
	if (!prg->task)
		return ELIMIT;

	/*
	 * Create the stack address space area.
	 */
	uintptr_t virt = (uintptr_t) -1;
	uintptr_t bound = USER_ADDRESS_SPACE_END - (STACK_SIZE_USER - 1);

	/* Adjust bound to create space for the desired guard page. */
	bound -= PAGE_SIZE;

	as_area_t *area = as_area_create(as,
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE | AS_AREA_GUARD |
	    AS_AREA_LATE_RESERVE, STACK_SIZE_USER, AS_AREA_ATTR_NONE,
	    &anon_backend, NULL, &virt, bound);
	if (!area) {
		task_destroy(prg->task);
		prg->task = NULL;
		return ENOMEM;
	}

	uspace_arg_t *kernel_uarg = (uspace_arg_t *)
	    malloc(sizeof(uspace_arg_t), 0);

	kernel_uarg->uspace_entry = (void *) entry_addr;
	kernel_uarg->uspace_stack = (void *) virt;
	kernel_uarg->uspace_stack_size = STACK_SIZE_USER;
	kernel_uarg->uspace_thread_function = NULL;
	kernel_uarg->uspace_thread_arg = NULL;
	kernel_uarg->uspace_uarg = NULL;

	/*
	 * Create the main thread.
	 */
	prg->main_thread = thread_create(uinit, kernel_uarg, prg->task,
	    THREAD_FLAG_USPACE, "uinit");
	if (!prg->main_thread) {
		free(kernel_uarg);
		as_area_destroy(as, virt);
		task_destroy(prg->task);
		prg->task = NULL;
		return ELIMIT;
	}

	return EOK;
}

/** Parse an executable image in the kernel memory.
 *
 * If the image belongs to a program loader, it is registered as such,
 * (and *task is set to NULL). Otherwise a task is created from the
 * executable image. The task is returned in *task.
 *
 * @param[in]  image_addr Address of an executable program image.
 * @param[in]  name       Name to set for the program's task.
 * @param[out] prg        Buffer for storing program info.
 *                        If image_addr points to a loader image,
 *                        prg->task will be set to NULL and EOK
 *                        will be returned.
 *
 * @return EOK on success or an error code.
 *
 */
errno_t program_create_from_image(void *image_addr, char *name, program_t *prg)
{
	as_t *as = as_create(0);
	if (!as)
		return ENOMEM;

	prg->loader_status = elf_load((elf_header_t *) image_addr, as);
	if (prg->loader_status != EE_OK) {
		as_destroy(as);
		prg->task = NULL;
		prg->main_thread = NULL;
		return ENOTSUP;
	}

	return program_create(as, ((elf_header_t *) image_addr)->e_entry,
	    name, prg);
}

/** Create a task from the program loader image.
 *
 * @param prg  Buffer for storing program info.
 * @param name Name to set for the program's task.
 *
 * @return EOK on success or an error code.
 *
 */
errno_t program_create_loader(program_t *prg, char *name)
{
	as_t *as = as_create(0);
	if (!as)
		return ENOMEM;

	void *loader = program_loader;
	if (!loader) {
		as_destroy(as);
		log(LF_OTHER, LVL_ERROR,
		    "Cannot spawn loader as none was registered");
		return ENOENT;
	}

	prg->loader_status = elf_load((elf_header_t *) program_loader, as);
	if (prg->loader_status != EE_OK) {
		as_destroy(as);
		log(LF_OTHER, LVL_ERROR, "Cannot spawn loader (%s)",
		    elf_error(prg->loader_status));
		return ENOENT;
	}

	return program_create(as, ((elf_header_t *) program_loader)->e_entry,
	    name, prg);
}

/** Make program ready.
 *
 * Switch program's main thread to the ready state.
 *
 * @param prg Program to make ready.
 *
 */
void program_ready(program_t *prg)
{
	thread_ready(prg->main_thread);
}

/** Syscall for creating a new loader instance from userspace.
 *
 * Creates a new task from the program loader image and sets
 * the task name.
 *
 * @param uspace_name Name to set on the new task (typically the same
 *                    as the command used to execute it).
 * @param name_len    Length of the name.
 *
 * @return EOK on success or an error code from @ref errno.h.
 *
 */
sys_errno_t sys_program_spawn_loader(char *uspace_name, size_t name_len)
{
	/* Cap length of name and copy it from userspace. */
	if (name_len > TASK_NAME_BUFLEN - 1)
		name_len = TASK_NAME_BUFLEN - 1;

	char namebuf[TASK_NAME_BUFLEN];
	errno_t rc = copy_from_uspace(namebuf, uspace_name, name_len);
	if (rc != EOK)
		return (sys_errno_t) rc;

	namebuf[name_len] = 0;

	/* Spawn the new task. */
	program_t prg;
	rc = program_create_loader(&prg, namebuf);
	if (rc != EOK)
		return rc;

	// FIXME: control the permissions
	perm_set(prg.task, perm_get(TASK));
	program_ready(&prg);

	return EOK;
}

/** @}
 */
