/*
 * SPDX-FileCopyrightText: 2016 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */

/**
 * @file
 * @brief	Userspace ELF loader.
 */

#include <elf/elf_load.h>
#include <elf/elf_mod.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <vfs/vfs.h>

#ifdef CONFIG_RTLD
#include <rtld/rtld.h>
#endif

#define DPRINTF(...)

/** Load ELF program.
 *
 * @param file File handle
 * @param info Place to store ELF program information
 * @return EOK on success or an error code
 */
errno_t elf_load(int file, elf_info_t *info)
{
#ifdef CONFIG_RTLD
	rtld_t *env;
#endif
	errno_t rc;

	rc = elf_load_file(file, 0, &info->finfo);
	if (rc != EOK) {
		DPRINTF("Failed to load executable '%s'.\n", file_name);
		return rc;
	}

	if (info->finfo.dynamic == NULL) {
		/* Statically linked program */
		DPRINTF("Binary is statically linked.\n");
		info->env = NULL;
		return EOK;
	}

	DPRINTF("Binary is dynamically linked.\n");
#ifdef CONFIG_RTLD
	DPRINTF("- prog dynamic: %p\n", info->finfo.dynamic);

	rc = rtld_prog_process(&info->finfo, &env);
	info->env = env;
#else
	rc = ENOTSUP;
#endif
	return rc;
}

/** Set ELF-related PCB entries.
 *
 * Fills the program control block @a pcb with information from
 * @a info.
 *
 * @param info	Program info structure
 * @param pcb PCB
 */
void elf_set_pcb(elf_info_t *info, pcb_t *pcb)
{
	pcb->entry = info->finfo.entry;
	pcb->dynamic = info->finfo.dynamic;
	pcb->rtld_runtime = info->env;
}

/** @}
 */
