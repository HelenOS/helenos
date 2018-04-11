/*
 * Copyright (c) 2006 Sergey Bondari
 * Copyright (c) 2006 Jakub Jermar
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

/** @addtogroup generic
 * @{
 */

/**
 * @file
 * @brief	Userspace ELF module loader.
 *
 * This module allows loading ELF binaries (both executables and
 * shared objects) from VFS. The current implementation allocates
 * anonymous memory, fills it with segment data and then adjusts
 * the memory areas' flags to the final value. In the future,
 * the segments will be mapped directly from the file.
 */

#include <errno.h>
#include <stdio.h>
#include <vfs/vfs.h>
#include <stddef.h>
#include <stdint.h>
#include <align.h>
#include <assert.h>
#include <as.h>
#include <elf/elf.h>
#include <smc.h>
#include <loader/pcb.h>
#include <entry_point.h>
#include <str_error.h>
#include <stdlib.h>

#include <elf/elf_load.h>

#define DPRINTF(...)

static const char *error_codes[] = {
	"no error",
	"invalid image",
	"address space error",
	"incompatible image",
	"unsupported image type",
	"irrecoverable error",
	"file io error"
};

static unsigned int elf_load_module(elf_ld_t *elf, size_t so_bias);
static int segment_header(elf_ld_t *elf, elf_segment_header_t *entry);
static int load_segment(elf_ld_t *elf, elf_segment_header_t *entry);

/** Load ELF binary from a file.
 *
 * Load an ELF binary from the specified file. If the file is
 * an executable program, it is loaded unbiased. If it is a shared
 * object, it is loaded with the bias @a so_bias. Some information
 * extracted from the binary is stored in a elf_info_t structure
 * pointed to by @a info.
 *
 * @param file      ELF file.
 * @param so_bias   Bias to use if the file is a shared object.
 * @param info      Pointer to a structure for storing information
 *                  extracted from the binary.
 *
 * @return EE_OK on success or EE_xx error code.
 *
 */
int elf_load_file(int file, size_t so_bias, eld_flags_t flags, elf_finfo_t *info)
{
	elf_ld_t elf;

	int ofile;
	errno_t rc = vfs_clone(file, -1, true, &ofile);
	if (rc == EOK) {
		rc = vfs_open(ofile, MODE_READ);
	}
	if (rc != EOK) {
		return EE_IO;
	}

	elf.fd = ofile;
	elf.info = info;
	elf.flags = flags;

	int ret = elf_load_module(&elf, so_bias);

	vfs_put(ofile);
	return ret;
}

int elf_load_file_name(const char *path, size_t so_bias, eld_flags_t flags,
    elf_finfo_t *info)
{
	int file;
	errno_t rc = vfs_lookup(path, 0, &file);
	if (rc == EOK) {
		int ret = elf_load_file(file, so_bias, flags, info);
		vfs_put(file);
		return ret;
	} else {
		return EE_IO;
	}
}

/** Load an ELF binary.
 *
 * The @a elf structure contains the loader state, including
 * an open file, from which the binary will be loaded,
 * a pointer to the @c info structure etc.
 *
 * @param elf		Pointer to loader state buffer.
 * @param so_bias	Bias to use if the file is a shared object.
 * @return EE_OK on success or EE_xx error code.
 */
static unsigned int elf_load_module(elf_ld_t *elf, size_t so_bias)
{
	elf_header_t header_buf;
	elf_header_t *header = &header_buf;
	aoff64_t pos = 0;
	size_t nr;
	int i, ret;
	errno_t rc;

	rc = vfs_read(elf->fd, &pos, header, sizeof(elf_header_t), &nr);
	if (rc != EOK || nr != sizeof(elf_header_t)) {
		DPRINTF("Read error.\n");
		return EE_IO;
	}

	elf->header = header;

	/* Identify ELF */
	if (header->e_ident[EI_MAG0] != ELFMAG0 ||
	    header->e_ident[EI_MAG1] != ELFMAG1 ||
	    header->e_ident[EI_MAG2] != ELFMAG2 ||
	    header->e_ident[EI_MAG3] != ELFMAG3) {
		DPRINTF("Invalid header.\n");
		return EE_INVALID;
	}

	/* Identify ELF compatibility */
	if (header->e_ident[EI_DATA] != ELF_DATA_ENCODING ||
	    header->e_machine != ELF_MACHINE ||
	    header->e_ident[EI_VERSION] != EV_CURRENT ||
	    header->e_version != EV_CURRENT ||
	    header->e_ident[EI_CLASS] != ELF_CLASS) {
		DPRINTF("Incompatible data/version/class.\n");
		return EE_INCOMPATIBLE;
	}

	if (header->e_phentsize != sizeof(elf_segment_header_t)) {
		DPRINTF("e_phentsize: %u != %zu\n", header->e_phentsize,
		    sizeof(elf_segment_header_t));
		return EE_INCOMPATIBLE;
	}

	/* Check if the object type is supported. */
	if (header->e_type != ET_EXEC && header->e_type != ET_DYN) {
		DPRINTF("Object type %d is not supported\n", header->e_type);
		return EE_UNSUPPORTED;
	}

	/* Shared objects can be loaded with a bias */
	if (header->e_type == ET_DYN)
		elf->bias = so_bias;
	else
		elf->bias = 0;

	elf->info->interp = NULL;
	elf->info->dynamic = NULL;

	/* Walk through all segment headers and process them. */
	for (i = 0; i < header->e_phnum; i++) {
		elf_segment_header_t segment_hdr;

		pos = header->e_phoff + i * sizeof(elf_segment_header_t);
		rc = vfs_read(elf->fd, &pos, &segment_hdr,
		    sizeof(elf_segment_header_t), &nr);
		if (rc != EOK || nr != sizeof(elf_segment_header_t)) {
			DPRINTF("Read error.\n");
			return EE_IO;
		}

		ret = segment_header(elf, &segment_hdr);
		if (ret != EE_OK)
			return ret;
	}

	elf->info->entry =
	    (entry_point_t)((uint8_t *)header->e_entry + elf->bias);

	DPRINTF("Done.\n");

	return EE_OK;
}

/** Print error message according to error code.
 *
 * @param rc Return code returned by elf_load().
 *
 * @return NULL terminated description of error.
 */
const char *elf_error(unsigned int rc)
{
	assert(rc < sizeof(error_codes) / sizeof(char *));

	return error_codes[rc];
}

/** Process TLS program header.
 *
 * @param elf  Pointer to loader state buffer.
 * @param hdr  TLS program header
 * @param info Place to store TLS info
 */
static void tls_program_header(elf_ld_t *elf, elf_segment_header_t *hdr,
    elf_tls_info_t *info)
{
	info->tdata = (void *)((uint8_t *)hdr->p_vaddr + elf->bias);
	info->tdata_size = hdr->p_filesz;
	info->tbss_size = hdr->p_memsz - hdr->p_filesz;
	info->tls_align = hdr->p_align;
}

/** Process segment header.
 *
 * @param elf   Pointer to loader state buffer.
 * @param entry	Segment header.
 *
 * @return EE_OK on success, error code otherwise.
 */
static int segment_header(elf_ld_t *elf, elf_segment_header_t *entry)
{
	switch (entry->p_type) {
	case PT_NULL:
	case PT_PHDR:
	case PT_NOTE:
		break;
	case PT_LOAD:
		return load_segment(elf, entry);
		break;
	case PT_INTERP:
		elf->info->interp =
		    (void *)((uint8_t *)entry->p_vaddr + elf->bias);

		// FIXME: This actually won't work, because the text segment is
		// not loaded yet.
#if 0
		if (elf->info->interp[entry->p_filesz - 1] != '\0') {
			DPRINTF("Unterminated ELF interp string.\n");
			return EE_INVALID;
		}
		DPRINTF("interpreter: \"%s\"\n", elf->info->interp);
#endif
		break;
	case PT_DYNAMIC:
		/* Record pointer to dynamic section into info structure */
		elf->info->dynamic =
		    (void *)((uint8_t *)entry->p_vaddr + elf->bias);
		DPRINTF("dynamic section found at %p\n",
		    (void *)elf->info->dynamic);
		break;
	case 0x70000000:
		/* FIXME: MIPS reginfo */
		break;
	case PT_TLS:
		/* Parse TLS program header */
		tls_program_header(elf, entry, &elf->info->tls);
		DPRINTF("TLS header found at %p\n",
		    (void *)((uint8_t *)entry->p_vaddr + elf->bias));
		break;
	case PT_SHLIB:
//	case PT_LOPROC:
//	case PT_HIPROC:
	default:
		DPRINTF("Segment p_type %d unknown.\n", entry->p_type);
		return EE_UNSUPPORTED;
		break;
	}
	return EE_OK;
}

/** Load segment described by program header entry.
 *
 * @param elf	Loader state.
 * @param entry Program header entry describing segment to be loaded.
 *
 * @return EE_OK on success, error code otherwise.
 */
int load_segment(elf_ld_t *elf, elf_segment_header_t *entry)
{
	void *a;
	int flags = 0;
	uintptr_t bias;
	uintptr_t base;
	void *seg_ptr;
	uintptr_t seg_addr;
	size_t mem_sz;
	aoff64_t pos;
	errno_t rc;
	size_t nr;

	bias = elf->bias;

	seg_addr = entry->p_vaddr + bias;
	seg_ptr = (void *) seg_addr;

	DPRINTF("Load segment at addr %p, size 0x%zx\n", (void *) seg_addr,
	    entry->p_memsz);

	if (entry->p_align > 1) {
		if ((entry->p_offset % entry->p_align) !=
		    (seg_addr % entry->p_align)) {
			DPRINTF("Align check 1 failed offset%%align=0x%zx, "
			    "vaddr%%align=0x%zx\n",
			    entry->p_offset % entry->p_align,
			    seg_addr % entry->p_align);
			return EE_INVALID;
		}
	}

	/* Final flags that will be set for the memory area */

	if (entry->p_flags & PF_X)
		flags |= AS_AREA_EXEC;
	if (entry->p_flags & PF_W)
		flags |= AS_AREA_WRITE;
	if (entry->p_flags & PF_R)
		flags |= AS_AREA_READ;
	flags |= AS_AREA_CACHEABLE;

	base = ALIGN_DOWN(entry->p_vaddr, PAGE_SIZE);
	mem_sz = entry->p_memsz + (entry->p_vaddr - base);

	DPRINTF("Map to seg_addr=%p-%p.\n", (void *) seg_addr,
	    (void *) (entry->p_vaddr + bias +
	    ALIGN_UP(entry->p_memsz, PAGE_SIZE)));

	/*
	 * For the course of loading, the area needs to be readable
	 * and writeable.
	 */
	a = as_area_create((uint8_t *) base + bias, mem_sz,
	    AS_AREA_READ | AS_AREA_WRITE | AS_AREA_CACHEABLE,
	    AS_AREA_UNPAGED);
	if (a == AS_MAP_FAILED) {
		DPRINTF("memory mapping failed (%p, %zu)\n",
		    (void *) (base + bias), mem_sz);
		return EE_MEMORY;
	}

	DPRINTF("as_area_create(%p, %#zx, %d) -> %p\n",
	    (void *) (base + bias), mem_sz, flags, (void *) a);

	/*
	 * Load segment data
	 */
	pos = entry->p_offset;
	rc = vfs_read(elf->fd, &pos, seg_ptr, entry->p_filesz, &nr);
	if (rc != EOK || nr != entry->p_filesz) {
		DPRINTF("read error\n");
		return EE_IO;
	}

	/*
	 * The caller wants to modify the segments first. He will then
	 * need to set the right access mode and ensure SMC coherence.
	 */
	if ((elf->flags & ELDF_RW) != 0)
		return EE_OK;

//	printf("set area flags to %d\n", flags);
	rc = as_area_change_flags(seg_ptr, flags);
	if (rc != EOK) {
		DPRINTF("Failed to set memory area flags.\n");
		return EE_MEMORY;
	}

	if (flags & AS_AREA_EXEC) {
		/* Enforce SMC coherence for the segment */
		if (smc_coherence(seg_ptr, entry->p_filesz))
			return EE_MEMORY;
	}

	return EE_OK;
}

/** @}
 */
