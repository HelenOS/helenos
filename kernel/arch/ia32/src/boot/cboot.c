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

/** @addtogroup ia32
 * @{
 */
/** @file
 */

#include <main/main.h>
#include <arch/boot/boot.h>
#include <arch/boot/cboot.h>
#include <arch/boot/memmap.h>
#include <config.h>
#include <memstr.h>
#include <string.h>
#include <macros.h>

/* This is a symbol so the type is only dummy. Obtain the value using &. */
extern int _hardcoded_unmapped_size;

/** Extract command name from the multiboot module command line.
 *
 * @param buf      Destination buffer (will always null-terminate).
 * @param n        Size of destination buffer.
 * @param cmd_line Input string (the command line).
 *
 */
static void extract_command(char *buf, size_t n, const char *cmd_line)
{
	const char *start, *end, *cp;
	size_t max_len;
	
	/* Find the first space. */
	end = strchr(cmd_line, ' ');
	if (end == NULL)
		end = cmd_line + strlen(cmd_line);
	
	/*
	 * Find last occurence of '/' before 'end'. If found, place start at
	 * next character. Otherwise, place start at beginning of buffer.
	 */
	cp = end;
	start = buf;
	while (cp != start) {
		if (*cp == '/') {
			start = cp + 1;
			break;
		}
		--cp;
	}
	
	/* Copy the command and null-terminate the string. */
	max_len = min(n - 1, (size_t) (end - start));
	strncpy(buf, start, max_len + 1);
	buf[max_len] = '\0';
}

/** C part of ia32 boot sequence.
 *
 * @param signature Should contain the multiboot signature.
 * @param mi        Pointer to the multiboot information structure.
 */
void ia32_cboot(uint32_t signature, const mb_info_t *mi)
{
	uint32_t flags;
	mb_mod_t *mods;
	uint32_t i;
	
	if (signature == MULTIBOOT_LOADER_MAGIC)
		flags = mi->flags;
	else {
		/* No multiboot info available. */
		flags = 0;
	}
	
	/* Copy module information. */
	
	if ((flags & MBINFO_FLAGS_MODS) != 0) {
		init.cnt = mi->mods_count;
		mods = mi->mods_addr;
		
		for (i = 0; i < init.cnt; i++) {
			init.tasks[i].addr = mods[i].start + 0x80000000;
			init.tasks[i].size = mods[i].end - mods[i].start;
			
			/* Copy command line, if available. */
			if (mods[i].string) {
				extract_command(init.tasks[i].name,
				    CONFIG_TASK_NAME_BUFLEN,
				    mods[i].string);
			} else
				init.tasks[i].name[0] = '\0';
		}
	} else
		init.cnt = 0;
	
	/* Copy memory map. */
	
	int32_t mmap_length;
	mb_mmap_t *mme;
	uint32_t size;
	
	if ((flags & MBINFO_FLAGS_MMAP) != 0) {
		mmap_length = mi->mmap_length;
		mme = mi->mmap_addr;
		e820counter = 0;
		
		i = 0;
		while (mmap_length > 0) {
			e820table[i++] = mme->mm_info;
			
			/* Compute address of next structure. */
			size = sizeof(mme->size) + mme->size;
			mme = ((void *) mme) + size;
			mmap_length -= size;
		}
		
		e820counter = i;
	} else
		e820counter = 0;
	
#ifdef CONFIG_SMP
	/* Copy AP bootstrap routines below 1 MB. */
	memcpy((void *) AP_BOOT_OFFSET, (void *) BOOT_OFFSET,
	    (size_t) &_hardcoded_unmapped_size);
#endif
	
	main_bsp();
}

/** @}
 */
