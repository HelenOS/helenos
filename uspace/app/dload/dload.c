/*
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

/** @addtogroup rtld rtld
 * @brief
 * @{
 */ 
/**
 * @file
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <loader/pcb.h>
#include <ipc/ipc.h>
#include <vfs/vfs.h>

/* from librtld */
#include <rtld.h>
#include <dynamic.h>
#include <elf_load.h>
#include <module.h>

void program_run(void *entry, pcb_t *pcb);

runtime_env_t dload_re;

int main(int argc, char *argv[])
{
	static module_t prog;

	runtime_env = &dload_re;

	DPRINTF("Hello, world! (from dload)\n");
	if (__pcb->dynamic == NULL) {
		printf("This is the dynamic loader. It is not supposed "
		    "to be executed directly.\n");
		return -1;
	}

	/*
	 * First we need to process dynamic sections of the executable
	 * program and insert it into the module graph.
	 */

	DPRINTF("Parse program .dynamic section at 0x%x\n", __pcb->dynamic);
	dynamic_parse(__pcb->dynamic, 0, &prog.dyn);
	prog.bias = 0;
	prog.dyn.soname = "[program]";

	/* Initialize list of loaded modules */
	list_initialize(&runtime_env->modules_head);
	list_append(&prog.modules_link, &runtime_env->modules_head);

	/* Pointer to program module. Used as root of the module graph. */
	runtime_env->program = &prog;

	/* Work around non-existent memory space allocation. */
	runtime_env->next_bias = 0x1000000;

	/*
	 * Now we can continue with loading all other modules.
	 */

	DPRINTF("Load all program dependencies\n");
	module_load_deps(&prog);

	/*
	 * Now relocate/link all modules together.
	 */

	/* Process relocations in all modules */
	DPRINTF("Relocate all modules\n");
	modules_process_relocs(&prog);

	/* Pass runtime evironment pointer through PCB. */
	__pcb->rtld_runtime = (void *) runtime_env;

	/*
	 * Finally, run the main program.
	 */
	DPRINTF("Run program.. (at 0x%lx)\n", (uintptr_t)__pcb->entry);

#ifndef RTLD_DEBUG
	ipc_hangup(fphone(stdout));
#endif
	program_run(__pcb->entry, __pcb);

	/* not reached */
	return 0;
}

/** @}
 */
