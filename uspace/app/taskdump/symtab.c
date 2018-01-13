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

/** @addtogroup debug
 * @{
 */
/** @file Handling of ELF symbol tables.
 *
 * This module allows one to load a symbol table from an ELF file and
 * use it to lookup symbol names/addresses in both directions.
 */

#include <elf/elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <str.h>
#include <str_error.h>
#include <vfs/vfs.h>

#include "include/symtab.h"

static errno_t elf_hdr_check(elf_header_t *hdr);
static errno_t section_hdr_load(int fd, const elf_header_t *ehdr, int idx,
    elf_section_header_t *shdr);
static errno_t chunk_load(int fd, off64_t start, size_t size, void **ptr);

/** Load symbol table from an ELF file.
 *
 * @param file_name	Name of the ELF file to read from.
 * @param symtab	Place to save pointer to new symtab structure.
 *
 * @return		EOK on success, ENOENT if file could not be open,
 *			ENOTSUP if file parsing failed.
 */
errno_t symtab_load(const char *file_name, symtab_t **symtab)
{
	symtab_t *stab;
	elf_header_t elf_hdr;
	elf_section_header_t sec_hdr;
	off64_t shstrt_start;
	size_t shstrt_size;
	char *shstrt, *sec_name;
	void *data;
	aoff64_t pos = 0;

	int fd;
	errno_t rc;
	size_t nread;
	int i;

	bool load_sec, sec_is_symtab;

	*symtab = NULL;

	stab = calloc(1, sizeof(symtab_t));
	if (stab == NULL)
		return ENOMEM;

	rc = vfs_lookup_open(file_name, WALK_REGULAR, MODE_READ, &fd);
	if (rc != EOK) {
		printf("failed opening file '%s': %s\n", file_name, str_error(rc));
		free(stab);
		return ENOENT;
	}

	rc = vfs_read(fd, &pos, &elf_hdr, sizeof(elf_header_t), &nread);
	if (rc != EOK || nread != sizeof(elf_header_t)) {
		printf("failed reading elf header\n");
		free(stab);
		return EIO;
	}

	rc = elf_hdr_check(&elf_hdr);
	if (rc != EOK) {
		printf("failed header check\n");
		free(stab);
		return ENOTSUP;
	}

	/*
	 * Load section header string table.
	 */

	rc = section_hdr_load(fd, &elf_hdr, elf_hdr.e_shstrndx, &sec_hdr);
	if (rc != EOK) {
		printf("failed reading shstrt header\n");
		free(stab);
		return ENOTSUP;
	}

	shstrt_start = sec_hdr.sh_offset;
	shstrt_size = sec_hdr.sh_size;

	rc = chunk_load(fd, shstrt_start, shstrt_size, (void **) &shstrt);
	if (rc != EOK) {
		printf("failed loading shstrt\n");
		free(stab);
		return ENOTSUP;
	}

	/* Read all section headers. */
	for (i = 0; i < elf_hdr.e_shnum; ++i) {
		rc = section_hdr_load(fd, &elf_hdr, i, &sec_hdr);
		if (rc != EOK) {
			free(shstrt);
			free(stab);
			return ENOTSUP;
		}

		sec_name = shstrt + sec_hdr.sh_name;
		if (str_cmp(sec_name, ".symtab") == 0 &&
		    sec_hdr.sh_type == SHT_SYMTAB) {
			load_sec = true;
			sec_is_symtab = true;
		} else if (str_cmp(sec_name, ".strtab") == 0 &&
		    sec_hdr.sh_type == SHT_STRTAB) {
			load_sec = true;
			sec_is_symtab = false;
		} else {
			load_sec = false;
		}

		if (load_sec) {
			rc = chunk_load(fd, sec_hdr.sh_offset, sec_hdr.sh_size,
			    &data);
			if (rc != EOK) {
				free(shstrt);
				free(stab);
				return ENOTSUP;
			}

			if (sec_is_symtab) {
				stab->sym = data;
				stab->sym_size = sec_hdr.sh_size;
			} else {
				stab->strtab = data;
				stab->strtab_size = sec_hdr.sh_size;
			}
		}
	}

	free(shstrt);
	vfs_put(fd);

	if (stab->sym == NULL || stab->strtab == NULL) {
		/* Tables not found. */
		printf("Symbol table or string table section not found\n");
		free(stab);
		return ENOTSUP;
	}

	*symtab = stab;

	return EOK;
}

/** Delete a symtab structure.
 *
 * Deallocates all resources used by the symbol table.
 */
void symtab_delete(symtab_t *st)
{
	free(st->sym);
	st->sym = NULL;

	free(st->strtab);
	st->strtab = NULL;

	free(st);
}

/** Convert symbol name to address.
 *
 * @param st	Symbol table.
 * @param name	Name of the symbol.
 * @param addr	Place to store address for symbol, if found.
 *
 * @return	EOK on success, ENOENT if no such symbol was found.
 */
errno_t symtab_name_to_addr(symtab_t *st, const char *name, uintptr_t *addr)
{
	size_t i;
	char *sname;
	unsigned stype;

	for (i = 0; i < st->sym_size / sizeof(elf_symbol_t); ++i) {
		if (st->sym[i].st_name == 0)
			continue;

		stype = ELF_ST_TYPE(st->sym[i].st_info);
		if (stype != STT_OBJECT && stype != STT_FUNC)
			continue;

		sname = st->strtab + st->sym[i].st_name;

		if (str_cmp(sname, name) == 0) {
			*addr = st->sym[i].st_value;
			return EOK;
		}
	}

	return ENOENT;
}

/** Convert symbol address to name.
 *
 * This function finds the symbol which starts at the highest address
 * less than or equal to @a addr.
 *
 * @param st	Symbol table.
 * @param addr	Address for lookup.
 * @param name	Place to store pointer name of symbol, if found.
 *		This is valid while @a st exists.
 *
 * @return	EOK on success or ENOENT if no matching symbol was found.
 */
errno_t symtab_addr_to_name(symtab_t *st, uintptr_t addr, char **name,
    size_t *offs)
{
	size_t i;
	uintptr_t saddr, best_addr;
	char *sname, *best_name;
	unsigned stype;

	best_name = NULL;
	best_addr = 0;

	for (i = 0; i < st->sym_size / sizeof(elf_symbol_t); ++i) {
		if (st->sym[i].st_name == 0)
			continue;

		stype = ELF_ST_TYPE(st->sym[i].st_info);
		if (stype != STT_OBJECT && stype != STT_FUNC &&
		    stype != STT_NOTYPE) {
			continue;
		}

		saddr = st->sym[i].st_value;
		sname = st->strtab + st->sym[i].st_name;

		/* An ugly hack to filter out some special ARM symbols. */
		if (sname[0] == '$')
			continue;

		if (saddr <= addr && (best_name == NULL || saddr > best_addr)) {
			best_name = sname;
			best_addr = saddr;
		}
	}

	if (best_name == NULL)
		return ENOENT;

	*name = best_name;
	*offs = addr - best_addr;
	return EOK;
}

/** Check if ELF header is valid.
 *
 * @return	EOK on success or an error code.
 */
static errno_t elf_hdr_check(elf_header_t *ehdr)
{
	/* TODO */
	return EOK;
}

/** Load ELF section header.
 *
 * @param fd		File descriptor of ELF file.
 * @param elf_hdr	Pointer to ELF file header in memory.
 * @param idx		Index of section whose header to load (0 = first).
 * @param sec_hdr	Place to store section header data.
 *
 * @return		EOK on success or EIO if I/O failed.
 */
static errno_t section_hdr_load(int fd, const elf_header_t *elf_hdr, int idx,
    elf_section_header_t *sec_hdr)
{
	errno_t rc;
	size_t nread;
	aoff64_t pos = elf_hdr->e_shoff + idx * sizeof(elf_section_header_t);

	rc = vfs_read(fd, &pos, sec_hdr, sizeof(elf_section_header_t), &nread);
	if (rc != EOK || nread != sizeof(elf_section_header_t))
		return EIO;

	return EOK;
}

/** Load a segment of bytes from a file and return it as a new memory block.
 *
 * This function fails if it cannot read exactly @a size bytes from the file.
 *
 * @param fd		File to read from.
 * @param start		Position in file where to start reading.
 * @param size		Number of bytes to read.
 * @param ptr		Place to store pointer to newly allocated block.
 *
 * @return		EOK on success or EIO on failure.
 */
static errno_t chunk_load(int fd, off64_t start, size_t size, void **ptr)
{
	errno_t rc;
	size_t nread;
	aoff64_t pos = start;

	*ptr = malloc(size);
	if (*ptr == NULL) {
		printf("failed allocating memory\n");
		return ENOMEM;
	}

	rc = vfs_read(fd, &pos, *ptr, size, &nread);
	if (rc != EOK || nread != size) {
		printf("failed reading chunk\n");
		free(*ptr);
		*ptr = NULL;
		return EIO;
	}

	return EOK;
}

/** @}
 */
