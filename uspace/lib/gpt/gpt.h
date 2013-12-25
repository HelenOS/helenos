/*
 * Copyright (c) 2009 Jiri Svoboda
 * Copyright (c) 2011-2013 Dominik Taborsky
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

/** @addtogroup libgpt
 * @{
 */
/** @file
 */

#ifndef LIBGPT_GPT_H_
#define LIBGPT_GPT_H_

typedef enum {
	AT_REQ_PART = 0,
	AT_NO_BLOCK_IO,
	AT_LEGACY_BOOT,
	AT_UNDEFINED,
	AT_SPECIFIC = 48
} gpt_attr_t;

/** GPT header */
typedef struct {
	uint8_t efi_signature[8];
	uint8_t revision[4];
	uint32_t header_size;
	uint32_t header_crc32;
	uint32_t reserved;
	uint64_t current_lba;
	uint64_t alternate_lba;
	uint64_t first_usable_lba;
	uint64_t last_usable_lba;
	uint8_t disk_guid[16];
	uint64_t entry_lba;
	uint32_t fillries;
	uint32_t entry_size;
	uint32_t pe_array_crc32;
} __attribute__((packed)) gpt_header_t;

/** GPT partition entry */
typedef struct {
	uint8_t part_type[16];
	uint8_t part_id[16];
	uint64_t start_lba;
	uint64_t end_lba;
	uint64_t attributes;
	uint8_t part_name[72];
} __attribute__((packed)) gpt_entry_t;

#endif
