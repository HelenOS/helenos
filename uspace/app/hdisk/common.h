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

#ifndef __COMMON_H__
#define __COMMON_H__

#include <libmbr.h>
#include <libgpt.h>

typedef enum {
	LYT_NONE,
	LYT_MBR,
	LYT_GPT,
} layouts_t;

union label_data {
	mbr_label_t	*mbr;
	gpt_label_t	*gpt;
};

typedef struct label label_t;

struct label {
	layouts_t layout;
	union label_data data;
	unsigned int alignment;
	int (* destroy_label)(label_t *);
	int (* add_part)     (label_t *, tinput_t *);
	int (* delete_part)  (label_t *, tinput_t *);
	int (* new_label)    (label_t *);
	int (* print_parts)  (label_t *);
	int (* read_parts)   (label_t *, service_id_t);
	int (* write_parts)  (label_t *, service_id_t);
	int (* extra_funcs)  (label_t *, tinput_t *, service_id_t);
};

#endif

