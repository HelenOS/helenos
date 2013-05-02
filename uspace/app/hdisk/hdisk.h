/*
 * Copyright (c) 2012, 2013 Dominik Taborsky
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

 /** @addtogroup hdisk
 * @{
 */
/** @file
 */

#include "common.h"

typedef enum {
	LYT_NONE,
	LYT_MBR,
	LYT_GPT,
} LAYOUTS;

typedef struct table {
	LAYOUTS layout;
	union table_data data;
	int (* add_part)(tinput_t *, union table_data *);
	int (* delete_part)(tinput_t *, union table_data *);
	int (* new_table)(tinput_t *, union table_data *);
	int (* print_parts)();
	int (* write_parts)(service_id_t, union table_data *);
	int (* extra_funcs)(tinput_t *, service_id_t, union table_data *);
} table_t;

#define init_table() \
	table.layout = LYT_NONE

#define set_table_mbr(m) \
	table.data.mbr.mbr = (m)

#define set_table_mbr_parts(p) \
	table.data.mbr.parts = (p)

#define set_table_gpt(g) \
	table.data.gpt.gpt = (g)

#define set_table_gpt_parts(p) \
	table.data.gpt.parts = (p)
