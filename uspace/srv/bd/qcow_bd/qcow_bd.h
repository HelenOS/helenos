/*
 * Copyright (c) 2021 Erik Kučák
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

/** @addtogroup qcow_bd
 * @{
 */
/** @file QCOW block device driver definitions.
 */

#ifndef __QCOW_BD_H__
#define __QCOW_BD_H__

#include <stdio.h>
#include <stdlib.h>
#include <async.h>
#include <as.h>
#include <bd_srv.h>
#include <fibril_synch.h>
#include <loc.h>
#include <stddef.h>
#include <stdint.h>
#include <errno.h>
#include <str_error.h>
#include <stdbool.h>
#include <task.h>
#include <macros.h>
#include <str.h>
#include <time.h>
#include <inttypes.h>

#define NAME "qcow_bd"
#define DEFAULT_BLOCK_SIZE 512
#define QCOW_MAGIC (('Q' << 24) | ('F' << 16) | ('I' << 8) | 0xfb)
#define QCOW_VERSION 1
#define QCOW_CRYPT_NONE 0
#define QCOW_OFLAG_COMPRESSED (1LL << 63)
#define QCOW_UNALLOCATED_REFERENCE 0

typedef struct __attribute__ ((__packed__)) QCowHeader {
    uint32_t magic;
    uint32_t version;
    uint64_t backing_file_offset;
    uint32_t backing_file_size;
    uint32_t mtime;
    uint64_t size;
    uint8_t cluster_bits;
    uint8_t l2_bits;
    uint16_t unused;
    uint32_t crypt_method;
    uint64_t l1_table_offset;
} QCowHeader;

typedef struct QcowState {
    uint64_t cluster_size;
	size_t block_size;
	aoff64_t num_blocks;
    uint64_t l2_size;
    uint64_t l1_size;
    uint64_t l1_table_offset;
    uint64_t *l2_references;
} QcowState;

#endif

/** @}
 */