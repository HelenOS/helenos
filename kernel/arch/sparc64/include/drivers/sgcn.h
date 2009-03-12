/*
 * Copyright (c) 2008 Pavel Rimsky
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

/** @addtogroup sparc64	
 * @{
 */
/** @file
 */

#ifndef KERN_sparc64_SGCN_H_
#define KERN_sparc64_SGCN_H_

#include <arch/types.h>
#include <console/chardev.h>

/* number of bytes in the TOC magic, including the terminating '\0' */
#define TOC_MAGIC_BYTES		8

/* number of bytes in the TOC key, including the terminating '\0' */
#define TOC_KEY_SIZE		8

/* maximum number of entries in the SRAM table of contents */
#define MAX_TOC_ENTRIES		32

/* number of bytes in the SGCN buffer magic, including the terminating '\0' */
#define SGCN_MAGIC_BYTES	4

/**
 * Entry in the SRAM table of contents. Describes one segment of the SRAM
 * which serves a particular purpose (e.g. OBP serial console, Solaris serial
 * console, Solaris mailbox,...). 
 */
typedef struct {
	/** key (e.g. "OBPCONS", "SOLCONS", "SOLMBOX",...) */
	char key[TOC_KEY_SIZE];
	
	/** size of the segment in bytes */
	uint32_t size;
	
	/** offset of the segment within SRAM */
	uint32_t offset;
} __attribute ((packed)) toc_entry_t;

/**
 * SRAM table of contents. Describes all segments within the SRAM.
 */
typedef struct {
	/** hard-wired to "TOCSRAM" */
	char magic[TOC_MAGIC_BYTES];
	
	/** we don't need this */
	char unused[8];
	
	/** TOC entries */
	toc_entry_t keys[MAX_TOC_ENTRIES];
} __attribute__ ((packed)) iosram_toc_t;

/**
 * SGCN buffer header. It is placed at the very beginning of the SGCN
 * buffer. 
 */
typedef struct {
	/** hard-wired to "CON" */
	char magic[SGCN_MAGIC_BYTES];
	
	/** we don't need this */
	char unused[8];
	
	/** offset within the SGCN buffer of the input buffer start */
	uint32_t in_begin;
	
	/** offset within the SGCN buffer of the input buffer end */
	uint32_t in_end;
	
	/** offset within the SGCN buffer of the input buffer read pointer */
	uint32_t in_rdptr;
	
	/** offset within the SGCN buffer of the input buffer write pointer */
	uint32_t in_wrptr;

	/** offset within the SGCN buffer of the output buffer start */
	uint32_t out_begin;
	
	/** offset within the SGCN buffer of the output buffer end */
	uint32_t out_end;
	
	/** offset within the SGCN buffer of the output buffer read pointer */
	uint32_t out_rdptr;
	
	/** offset within the SGCN buffer of the output buffer write pointer */
	uint32_t out_wrptr;
} __attribute__ ((packed)) sgcn_buffer_header_t;

void sgcn_grab(void);
void sgcn_release(void);
indev_t *sgcnin_init(void);
void sgcnout_init(void);

#endif

/** @}
 */
