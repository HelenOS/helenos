/*
 * Copyright (c) 2011 Jiri Svoboda
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
 * Basically at the beginning there should be a note segment followed
 * by one loadable segment per memory area.
 *
 * The note segment contains a series of records with register state,
 * process info etc. We only write one record NT_PRSTATUS which contains
 * process/register state (anything which is not register state we fill
 * with zeroes).
 */

#include <align.h>
#include <elf/elf.h>
#include <elf/elf_linux.h>
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
#include <libarch/istate.h>

#include "elf_core.h"

static off64_t align_foff_up(off64_t, uintptr_t, size_t);
static int align_pos(int, size_t);
static int write_mem_area(int, as_area_info_t *, async_sess_t *);

#define BUFFER_SIZE 0x1000
static uint8_t buffer[BUFFER_SIZE];

/** Save ELF core file.
 *
 * @param file_name Name of file to save to.
 * @param ainfo     Array of @a n memory area info structures.
 * @param n         Number of memory areas.
 * @param sess      Debugging session.
 *
 * @return EOK on sucess.
 * @return ENOENT if file cannot be created.
 * @return ENOMEM on out of memory.
 * @return EIO on write error.
 *
 */
int elf_core_save(const char *file_name, as_area_info_t *ainfo, unsigned int n,
    async_sess_t *sess, istate_t *istate)
{
	elf_header_t elf_hdr;
	off64_t foff;
	size_t n_ph;
	elf_word flags;
	elf_segment_header_t *p_hdr;
	elf_prstatus_t pr_status;
	elf_note_t note;
	size_t word_size;

	int fd;
	ssize_t rc;
	unsigned int i;

#ifdef __32_BITS__
	word_size = 4;
#endif
#ifdef __64_BITS__
	/*
	 * This should be 8 per the 64-bit ELF spec, but the Linux kernel
	 * screws up and uses 4 anyway (and screws up elf_note_t as well)
	 * and we are trying to be compatible with Linux GDB target. Sigh.
	 */
	word_size = 4;
#endif
	memset(&pr_status, 0, sizeof(pr_status));
	istate_to_elf_regs(istate, &pr_status.regs);

	n_ph = n + 1;

	p_hdr = malloc(sizeof(elf_segment_header_t) * n_ph);
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
	 *	note segment
	 * repeat:
	 *	(pad for alignment)
	 *	core segment
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

	memset(&p_hdr[0], 0, sizeof(p_hdr[0]));
	p_hdr[0].p_type = PT_NOTE;
	p_hdr[0].p_offset = foff;
	p_hdr[0].p_vaddr = 0;
	p_hdr[0].p_paddr = 0;
	p_hdr[0].p_filesz = sizeof(elf_note_t)
	    + ALIGN_UP((str_size("CORE") + 1), word_size)
	    + ALIGN_UP(sizeof(elf_prstatus_t), word_size);
	p_hdr[0].p_memsz = 0;
	p_hdr[0].p_flags = 0;
	p_hdr[0].p_align = 1;

	foff += p_hdr[0].p_filesz;

	for (i = 0; i < n; ++i) {
		foff = align_foff_up(foff, ainfo[i].start_addr, PAGE_SIZE);

		flags = 0;
		if (ainfo[i].flags & AS_AREA_READ)
			flags |= PF_R;
		if (ainfo[i].flags & AS_AREA_WRITE)
			flags |= PF_W;
		if (ainfo[i].flags & AS_AREA_EXEC)
			flags |= PF_X;

		memset(&p_hdr[i + 1], 0, sizeof(p_hdr[i + 1]));
		p_hdr[i + 1].p_type = PT_LOAD;
		p_hdr[i + 1].p_offset = foff;
		p_hdr[i + 1].p_vaddr = ainfo[i].start_addr;
		p_hdr[i + 1].p_paddr = 0;
		p_hdr[i + 1].p_filesz = ainfo[i].size;
		p_hdr[i + 1].p_memsz = ainfo[i].size;
		p_hdr[i + 1].p_flags = flags;
		p_hdr[i + 1].p_align = PAGE_SIZE;

		foff += ainfo[i].size;
	}

	rc = write_all(fd, &elf_hdr, sizeof(elf_hdr));
	if (rc != sizeof(elf_hdr)) {
		printf("Failed writing ELF header.\n");
		free(p_hdr);
		return EIO;
	}

	for (i = 0; i < n_ph; ++i) {
		rc = write_all(fd, &p_hdr[i], sizeof(p_hdr[i]));
		if (rc != sizeof(p_hdr[i])) {
			printf("Failed writing program header.\n");
			free(p_hdr);
			return EIO;
		}
	}

	if (lseek(fd, p_hdr[0].p_offset, SEEK_SET) == (off64_t) -1) {
		printf("Failed writing memory data.\n");
		free(p_hdr);
		return EIO;
	}

	/*
	 * Write note header
	 */
	note.namesz = str_size("CORE") + 1;
	note.descsz = sizeof(elf_prstatus_t);
	note.type = NT_PRSTATUS;

	rc = write_all(fd, &note, sizeof(elf_note_t));
	if (rc != sizeof(elf_note_t)) {
		printf("Failed writing note header.\n");
		free(p_hdr);
		return EIO;
	}

	rc = write_all(fd, "CORE", note.namesz);
	if (rc != (ssize_t) note.namesz) {
		printf("Failed writing note header.\n");
		free(p_hdr);
		return EIO;
	}

	rc = align_pos(fd, word_size);
	if (rc != EOK) {
		printf("Failed writing note header.\n");
		free(p_hdr);
		return EIO;
	}

	rc = write_all(fd, &pr_status, sizeof(elf_prstatus_t));
	if (rc != sizeof(elf_prstatus_t)) {
		printf("Failed writing register data.\n");
		free(p_hdr);
		return EIO;
	}

	for (i = 1; i < n_ph; ++i) {
		if (lseek(fd, p_hdr[i].p_offset, SEEK_SET) == (off64_t) -1) {
			printf("Failed writing memory data.\n");
			free(p_hdr);
			return EIO;
		}
		if (write_mem_area(fd, &ainfo[i - 1], sess) != EOK) {
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
 * @param fd   File to write to.
 * @param area Memory area info structure.
 * @param sess Debugging session.
 *
 * @return EOK on success, EIO on failure.
 *
 */
static int write_mem_area(int fd, as_area_info_t *area, async_sess_t *sess)
{
	size_t to_copy;
	size_t total;
	uintptr_t addr;
	ssize_t rc;

	addr = area->start_addr;
	total = 0;

	while (total < area->size) {
		to_copy = min(area->size - total, BUFFER_SIZE);
		rc = udebug_mem_read(sess, buffer, addr, to_copy);
		if (rc < 0) {
			printf("Failed reading task memory.\n");
			return EIO;
		}

		rc = write_all(fd, buffer, to_copy);
		if (rc != (ssize_t) to_copy) {
			printf("Failed writing memory contents.\n");
			return EIO;
		}

		addr += to_copy;
		total += to_copy;
	}

	return EOK;
}

static int align_pos(int fd, size_t align)
{
	off64_t cur_pos;
	size_t rem, adv;

	cur_pos = lseek(fd, 0, SEEK_CUR);
	if (cur_pos < 0)
		return -1;

	rem = cur_pos % align;
	adv = align - rem;

	cur_pos = lseek(fd, adv, SEEK_CUR);
	if (cur_pos < 0)
		return -1;

	return EOK;
}

/** @}
 */
