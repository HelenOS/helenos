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

/** @addtogroup genarch
 * @{
 */
/** @file
 */

#include <genarch/multiboot/multiboot.h>
#include <arch/types.h>
#include <typedefs.h>
#include <config.h>
#include <str.h>
#include <macros.h>

/** Extract command name from the multiboot module command line.
 *
 * @param buf      Destination buffer (will always NULL-terminate).
 * @param sz       Size of destination buffer (in bytes).
 * @param cmd_line Input string (the command line).
 *
 */
static void extract_command(char *buf, size_t sz, const char *cmd_line)
{
	/* Find the first space. */
	const char *end = str_chr(cmd_line, ' ');
	if (end == NULL)
		end = cmd_line + str_size(cmd_line);
	
	/*
	 * Find last occurence of '/' before 'end'. If found, place start at
	 * next character. Otherwise, place start at beginning of buffer.
	 */
	const char *cp = end;
	const char *start = buf;
	
	while (cp != start) {
		if (*cp == '/') {
			start = cp + 1;
			break;
		}
		cp--;
	}
	
	/* Copy the command. */
	str_ncpy(buf, sz, start, (size_t) (end - start));
}

/** Parse multiboot information structure.
 *
 * If @a signature does not contain a valid multiboot signature,
 * assumes no multiboot information is available.
 *
 * @param signature Should contain the multiboot signature.
 * @param mi        Pointer to the multiboot information structure.
 */
void multiboot_info_parse(uint32_t signature, const multiboot_info_t *mi)
{
	uint32_t flags;
	
	if (signature == MULTIBOOT_LOADER_MAGIC)
		flags = mi->flags;
	else {
		/* No multiboot info available. */
		flags = 0;
	}
	
	/* Copy module information. */
	uint32_t i;
	if ((flags & MBINFO_FLAGS_MODS) != 0) {
		init.cnt = min(mi->mods_count, CONFIG_INIT_TASKS);
		multiboot_mod_t *mods
		    = (multiboot_mod_t *) MULTIBOOT_PTR(mi->mods_addr);
		
		for (i = 0; i < init.cnt; i++) {
			init.tasks[i].addr = PA2KA(mods[i].start);
			init.tasks[i].size = mods[i].end - mods[i].start;
			
			/* Copy command line, if available. */
			if (mods[i].string) {
				extract_command(init.tasks[i].name,
				    CONFIG_TASK_NAME_BUFLEN,
				    MULTIBOOT_PTR(mods[i].string));
			} else
				init.tasks[i].name[0] = 0;
		}
	} else
		init.cnt = 0;
	
	/* Copy memory map. */
	
	if ((flags & MBINFO_FLAGS_MMAP) != 0) {
		int32_t mmap_length = mi->mmap_length;
		multiboot_mmap_t *mme = MULTIBOOT_PTR(mi->mmap_addr);
		e820counter = 0;
		
		i = 0;
		while ((mmap_length > 0) && (i < MEMMAP_E820_MAX_RECORDS)) {
			e820table[i++] = mme->mm_info;
			
			/* Compute address of next structure. */
			uint32_t size = sizeof(mme->size) + mme->size;
			mme = ((void *) mme) + size;
			mmap_length -= size;
		}
		
		e820counter = i;
	} else
		e820counter = 0;
}

/** @}
 */
