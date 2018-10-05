/*
 * Copyright (c) 2008 Pavel Rimsky
 * Copyright (c) 2017 Jiri Svoboda
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

/** @addtogroup kernel_sparc64
 * @{
 */
/**
 * @file
 * @brief Niagara input/output buffer shared between kernel and user space
 */

#ifdef KERNEL
#include <mm/as.h>
#else
#include <as.h>
#endif

#include <stdint.h>

#define OUTPUT_BUFFER_SIZE  ((PAGE_SIZE) - 2 * 8)

typedef struct {
	uint64_t read_ptr;
	uint64_t write_ptr;
	char data[OUTPUT_BUFFER_SIZE];
} __attribute__((packed)) niagara_output_buffer_t;

#define INPUT_BUFFER_SIZE  ((PAGE_SIZE) - 2 * 8)

typedef struct {
	uint64_t write_ptr;
	uint64_t read_ptr;
	char data[INPUT_BUFFER_SIZE];
} __attribute__((packed)) niagara_input_buffer_t;

/** @}
 */
