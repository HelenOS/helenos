/*
 * Copyright (c) 2006 Sergey Bondari
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

/** @addtogroup libc
 * @{
 */
/** @file
 * @brief ELF loader structures and public functions.
 */

#ifndef ELF_MOD_H_
#define ELF_MOD_H_

#include <elf/elf.h>
#include <stddef.h>
#include <stdint.h>
#include <loader/pcb.h>

typedef enum {
	/** Leave all segments in RW access mode. */
	ELDF_RW = 1
} eld_flags_t;

/** TLS info for a module */
typedef struct {
	/** tdata section image */
	void *tdata;
	/** Size of tdata section image in bytes */
	size_t tdata_size;
	/** Size of tbss section */
	size_t tbss_size;
	/** Alignment of TLS initialization image */
	size_t tls_align;
} elf_tls_info_t;

/**
 * Some data extracted from the headers are stored here
 */
typedef struct {
	/** Entry point */
	entry_point_t entry;

	/** The base address where the file has been loaded.
	 *  Points to the ELF file header.
	 */
	void *base;

	/** ELF interpreter name or NULL if statically-linked */
	const char *interp;

	/** Pointer to the dynamic section */
	void *dynamic;

	/** TLS info */
	elf_tls_info_t tls;
} elf_finfo_t;

/**
 * Holds information about an ELF binary being loaded.
 */
typedef struct {
	/** Filedescriptor of the file from which we are loading */
	int fd;

	/** Difference between run-time addresses and link-time addresses */
	uintptr_t bias;

	/** Flags passed to the ELF loader. */
	eld_flags_t flags;

	/** Store extracted info here */
	elf_finfo_t *info;
} elf_ld_t;

extern errno_t elf_load_file(int, eld_flags_t, elf_finfo_t *);
extern errno_t elf_load_file_name(const char *, eld_flags_t, elf_finfo_t *);

#endif

/** @}
 */
