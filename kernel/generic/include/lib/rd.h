/*
 * Copyright (C) 2006 Martin Decky
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

/** @addtogroup genericmm
 * @{
 */
/** @file
 */

#ifndef KERN_RD_H_
#define KERN_RD_H_

#include <arch/types.h>
#include <typedefs.h>

/**
 * RAM disk version
 */
#define	RD_VERSION	0

/**
 * RAM disk magic number
 */
#define RD_MAGIC_SIZE	4
#define RD_MAG0			'H'
#define RD_MAG1			'O'
#define RD_MAG2			'R'
#define RD_MAG3			'D'

/**
 * RAM disk data encoding types
 */
#define RD_DATA_NONE	0
#define RD_DATA_LSB		1		/* Least significant byte first (little endian) */
#define RD_DATA_MSB		2		/* Most signigicant byte first (big endian) */

/**
 * RAM disk error return codes
 */
#define RE_OK			0	/* No error */
#define RE_INVALID		1	/* Invalid RAM disk image */
#define RE_UNSUPPORTED		2	/* Non-supported image (e.g. wrong version) */

/** RAM disk header */
typedef struct {
	uint8_t magic[RD_MAGIC_SIZE];
	uint8_t version;
	uint8_t data_type;
	uint32_t header_size;
	uint64_t data_size;
} rd_header;

extern int init_rd(rd_header * addr);

#endif

/** @}
 */
