/*
 * Copyright (c) 2010 Jiri Svoboda
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

/** @addtogroup taskdump
 * @{
 */
/** @file Write ELF core files.
 *
 * Creates ELF core files. Core files do not seem to be specified by some
 * standard (the System V ABI explicitely states that it does not specify
 * them).
 *
 * Looking at core files produced by Linux, these don't have section headers,
 * only program headers, although objdump shows them as having sections.
 * Basically at the beginning there should be a note segment (which we
 * do not write) and one loadable segment per memory area (which we do write).
 *
 * The note segment probably contains register state, etc. -- we don't
 * deal with these yet. Nevertheless you can use these core files with
 * objdump or gdb.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <mem.h>
#include <stdint.h>
#include <as.h>
#include <udebug.h>
#include <macros.h>

#include <elf.h>
#include "include/elf_core.h"

static off64_t align_foff_up(off64_t foff, uintptr_t vaddr, size_t page_size);
static int write_all(int fd, void *data, size_t len);
static int write_mem_area(int fd, as_area_info_t *area, int phoneid);

#define BUFFER_SIZE 0x1000
static uint8_t buffer[BUFFER_SIZE];

/** Save ELF core file.
 *
 * @param file_name	Name of file to save to.
 * @param ainfo		Array of @a n memory area info structures.
 * @param n		Number of memory areas.
 * @param phoneid	Debugging phone.
 *
 * @return		EOK on sucess, ENOENT if file cannot be created,
 *			ENOMEM on out of memory, EIO on write error.
 */
int elf_core_save(const char *file_name, as_area_info_t *ainfo, unsigned int n, int phoneid)
{
	elf_header_t elf_hdr;
	off64_t foff;
	size_t n_ph;
	elf_word flags;
	elf_segment_header_t *p_hdr;

	int fd;
	int rc;
	unsigned int i;

	n_ph = n;

	p_hdr = malloc(sizeof(elf_segment_header_t) * n);
	if (p_hdr == NULL) {
		printf("Failed allocating memory.\n");
		return ENOMEM;
	}

	fd = open(file_name, O_CREAT | O_WRONLY, 0644);
	if (fd < 0) {
		printf("Failed opening file.\n");
		free(p_hdr);
		return ENOENT;
	}

	/*
	 * File layout:
	 *
	 * 	ELF header
	 *	program headers
	 * repeat:
	 *	(pad for alignment)
	 *	segment data
	 * end repeat
	 */

	memset(&elf_hdr, 0, sizeof(elf_hdr));
	elf_hdr.e_ident[EI_MAG0] = ELFMAG0;
	elf_hdr.e_ident[EI_MAG1] = ELFMAG1;
	elf_hdr.e_ident[EI_MAG2] = ELFMAG2;
	elf_hdr.e_ident[EI_MAG3] = ELFMAG3;
	elf_hdr.e_ident[EI_CLASS] = ELF_CLASS;
	elf_hdr.e_ident[EI_DATA] = ELF_DATA_ENCODING;
	elf_hdr.e_ident[EI_VERSION] = EV_CURRENT;

	elf_hdr.e_type = ET_CORE;
	elf_hdr.e_machine = ELF_MACHINE;
	elf_hdr.e_version = EV_CURRENT;
	elf_hdr.e_entry = 0;
	elf_hdr.e_phoff = sizeof(elf_header_t);
	elf_hdr.e_shoff = 0;
	elf_hdr.e_flags = 0;
	elf_hdr.e_ehsize = sizeof(elf_hdr);
	elf_hdr.e_phentsize = sizeof(elf_segment_header_t);
	elf_hdr.e_phnum = n_ph;
	elf_hdr.e_shentsize = 0;
	elf_hdr.e_shnum = 0;
	elf_hdr.e_shstrndx = 0;

	/* foff is used for allocation of file space for segment data. */
	foff = elf_hdr.e_phoff + n_ph * sizeof(elf_segment_header_t);

	for (i = 1; i <= n; ++i) {
		foff = align_foff_up(foff, ainfo[i - 1].start_addr, PAGE_SIZE);

		flags = 0;
		if (ainfo[i - 1].flags & AS_AREA_READ)
			flags |= PF_R;
		if (ainfo[i - 1].flags & AS_AREA_WRITE)
			flags |= PF_W;
		if (ainfo[i - 1].flags & AS_AREA_EXEC)
			flags |= PF_X;

		memset(&p_hdr[i - 1], 0, sizeof(p_hdr[i - 1]));
		p_hdr[i - 1].p_type = PT_LOAD;
		p_hdr[i - 1].p_offset = foff;
		p_hdr[i - 1].p_vaddr = ainfo[i - 1].start_addr;
		p_hdr[i - 1].p_paddr = 0;
		p_hdr[i - 1].p_filesz = ainfo[i - 1].size;
		p_hdr[i - 1].p_memsz = ainfo[i - 1].size;
		p_hdr[i - 1].p_flags = flags;
		p_hdr[i - 1].p_align = PAGE_SIZE;

		foff += ainfo[i - 1].size;
	}

	rc = write_all(fd, &elf_hdr, sizeof(elf_hdr));
	if (rc != EOK) {
		printf("Failed writing ELF header.\n");
		free(p_hdr);
		return EIO;
	}

	for (i = 0; i < n_ph; ++i) {
		rc = write_all(fd, &p_hdr[i], sizeof(p_hdr[i]));
		if (rc != EOK) {
			printf("Failed writing program header.\n");
			free(p_hdr);
			return EIO;
		}
	}

	for (i = 0; i < n_ph; ++i) {
		if (lseek(fd, p_hdr[i].p_offset, SEEK_SET) == (off64_t) -1) {
			printf("Failed writing memory data.\n");
			free(p_hdr);
			return EIO;
		}
		if (write_mem_area(fd, &ainfo[i], phoneid) != EOK) {
			printf("Failed writing memory data.\n");
			free(p_hdr);
			return EIO;
		}
	}

	free(p_hdr);

	return EOK;
}

/** Align file offset up to be congruent with vaddr modulo page size. */
static off64_t align_foff_up(off64_t foff, uintptr_t vaddr, size_t page_size)
{
	off64_t rva = vaddr % page_size;
	off64_t rfo = foff % page_size;
	
	if (rva >= rfo)
		return (foff + (rva - rfo));
	
	return (foff + (page_size + (rva - rfo)));
}

/** Write memory area from application to core file.
 *
 * @param fd		File to write to.
 * @param area		Memory area info structure.
 * @param phoneid	Debugging phone.
 *
 * @return		EOK on success, EIO on failure.
 */
static int write_mem_area(int fd, as_area_info_t *area, int phoneid)
{
	size_t to_copy;
	size_t total;
	uintptr_t addr;
	int rc;

	addr = area->start_addr;
	total = 0;

	while (total < area->size) {
		to_copy = min(area->size - total, BUFFER_SIZE);
		rc = udebug_mem_read(phoneid, buffer, addr, to_copy);
		if (rc < 0) {
			printf("Failed reading task memory.\n");
			return EIO;
		}

		rc = write_all(fd, buffer, to_copy);
		if (rc != EOK) {
			printf("Failed writing memory contents.\n");
			return EIO;
		}

		addr += to_copy;
		total += to_copy;
	}

	return EOK;
}

/** Write until the buffer is written in its entirety.
 *
 * This function fails if it cannot write exactly @a len bytes to the file.
 *
 * @param fd		The file to write to.
 * @param buf		Data, @a len bytes long.
 * @param len		Number of bytes to write.
 *
 * @return		EOK on error, return value from write() if writing
 *			failed.
 */
static int write_all(int fd, void *data, size_t len)
{
	int cnt = 0;

	do {
		data += cnt;
		len -= cnt;
		cnt = write(fd, data, len);
	} while (cnt > 0 && (len - cnt) > 0);

	if (cnt < 0)
		return cnt;

	if (len - cnt > 0)
		return EIO;

	return EOK;
}


/** @}
 */
