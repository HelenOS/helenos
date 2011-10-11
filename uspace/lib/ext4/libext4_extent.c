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

/**
 * @file	libext4_extent.c
 * @brief	TODO
 */

#include <byteorder.h>
#include "libext4_extent.h"

uint16_t ext4_extent_header_get_magic(ext4_extent_header_t *header)
{
	return uint16_t_le2host(header->magic);
}

uint16_t ext4_extent_header_get_entries_count(ext4_extent_header_t *header)
{
	return uint16_t_le2host(header->entries_count);
}

uint16_t ext4_extent_header_get_max_entries_count(ext4_extent_header_t *header)
{
	return uint16_t_le2host(header->max_entries_count);
}

uint16_t ext4_extent_header_get_depth(ext4_extent_header_t *header)
{
	return uint16_t_le2host(header->depth);
}

uint32_t ext4_extent_header_get_generation(ext4_extent_header_t *header)
{
	return uint32_t_le2host(header->generation);
}

/**
 * @}
 */ 
