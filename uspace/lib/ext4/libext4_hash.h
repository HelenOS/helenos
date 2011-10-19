/*
 * Copyright (c) 2011 Frantisek Princ
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

/** @addtogroup libext4
 * @{
 */ 

#ifndef LIBEXT4_LIBEXT4_HASH_H_
#define LIBEXT4_LIBEXT4_HASH_H_

#include <sys/types.h>

#define EXT4_HASH_VERSION_LEGACY			0
#define EXT4_HASH_VERSION_HALF_MD4			1
#define EXT4_HASH_VERSION_TEA				2
#define EXT4_HASH_VERSION_LEGACY_UNSIGNED	3
#define EXT4_HASH_VERSION_HALF_MD4_UNSIGNED	4
#define EXT4_HASH_VERSION_TEA_UNSIGNED		5

typedef struct ext4_hash_info {
	uint32_t hash;
	uint32_t minor_hash;
	uint32_t hash_version;
	uint32_t *seed;
} ext4_hash_info_t;

extern int ext4_hash_string(ext4_hash_info_t *, int, const char *);

#endif

/**
 * @}
 */
