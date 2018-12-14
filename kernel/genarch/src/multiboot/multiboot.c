/*
 * Copyright (c) 2009 Jiri Svoboda
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
 */

#include <typedefs.h>
#include <genarch/multiboot/multiboot.h>
#include <config.h>
#include <stddef.h>
#include <str.h>

/** Extract command name from the multiboot module command line.
 *
 * @param buf      Destination buffer (will be always NULL-terminated).
 * @param size     Size of destination buffer (in bytes).
 * @param cmd_line Input string (the command line).
 *
 */
void multiboot_extract_command(char *buf, size_t size, const char *cmd_line)
{
	/* Find the first space. */
	const char *end = str_chr(cmd_line, ' ');
	if (end == NULL)
		end = cmd_line + str_size(cmd_line);

	/*
	 * Find last occurence of '/' before 'end'. If found, place start at
	 * next character. Otherwise, place start at beginning of command line.
	 */
	const char *cp = end;
	const char *start = cmd_line;

	while (cp != start) {
		if (*cp == '/') {
			start = cp + 1;
			break;
		}
		cp--;
	}

	/* Copy the command. */
	str_ncpy(buf, size, start, (size_t) (end - start));
}

/** Extract arguments from the multiboot module command line.
 *
 * @param buf      Destination buffer (will be always NULL-terminated).
 * @param size     Size of destination buffer (in bytes).
 * @param cmd_line Input string (the command line).
 *
 */
void multiboot_extract_argument(char *buf, size_t size, const char *cmd_line)
{
	/* Start after first space. */
	const char *start = str_chr(cmd_line, ' ');
	if (start == NULL) {
		str_cpy(buf, size, "");
		return;
	}

	const char *end = cmd_line + str_size(cmd_line);

	/* Skip the space(s). */
	while (start != end) {
		if (start[0] == ' ')
			start++;
		else
			break;
	}

	str_ncpy(buf, size, start, (size_t) (end - start));
}

static void multiboot_modules(uint32_t count, multiboot_module_t *mods)
{
	for (uint32_t i = 0; i < count; i++) {
		if (init.cnt >= CONFIG_INIT_TASKS)
			break;

		init.tasks[init.cnt].paddr = mods[i].start;
		init.tasks[init.cnt].size = mods[i].end - mods[i].start;

		/* Copy command line, if available. */
		if (mods[i].string) {
			multiboot_extract_command(init.tasks[init.cnt].name,
			    CONFIG_TASK_NAME_BUFLEN, MULTIBOOT_PTR(mods[i].string));
			multiboot_extract_argument(init.tasks[init.cnt].arguments,
			    CONFIG_TASK_ARGUMENTS_BUFLEN, MULTIBOOT_PTR(mods[i].string));
		} else {
			init.tasks[init.cnt].name[0] = 0;
			init.tasks[init.cnt].arguments[0] = 0;
		}

		init.cnt++;
	}
}

static void multiboot_memmap(uint32_t length, multiboot_memmap_t *memmap)
{
	uint32_t pos = 0;

	while ((pos < length) && (e820counter < MEMMAP_E820_MAX_RECORDS)) {
		e820table[e820counter] = memmap->mm_info;

		/* Compute address of next structure. */
		uint32_t size = sizeof(memmap->size) + memmap->size;
		memmap = (multiboot_memmap_t *) ((uintptr_t) memmap + size);
		pos += size;

		e820counter++;
	}
}

/** Parse multiboot information structure.
 *
 * If @a signature does not contain a valid multiboot signature,
 * assumes no multiboot information is available.
 *
 * @param signature Should contain the multiboot signature.
 * @param info      Multiboot information structure.
 *
 */
void multiboot_info_parse(uint32_t signature, const multiboot_info_t *info)
{
	if (signature != MULTIBOOT_LOADER_MAGIC)
		return;

	/* Copy command line. */
	if ((info->flags & MULTIBOOT_INFO_FLAGS_CMDLINE) != 0)
		multiboot_cmdline((char *) MULTIBOOT_PTR(info->cmd_line));

	/* Copy modules information. */
	if ((info->flags & MULTIBOOT_INFO_FLAGS_MODS) != 0)
		multiboot_modules(info->mods_count,
		    (multiboot_module_t *) MULTIBOOT_PTR(info->mods_addr));

	/* Copy memory map. */
	if ((info->flags & MULTIBOOT_INFO_FLAGS_MMAP) != 0)
		multiboot_memmap(info->mmap_length,
		    (multiboot_memmap_t *) MULTIBOOT_PTR(info->mmap_addr));
}

/** @}
 */
