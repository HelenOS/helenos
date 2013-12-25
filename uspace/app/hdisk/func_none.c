/*
 * Copyright (c) 2012-2013 Dominik Taborsky
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

#include <errno.h>
#include "func_none.h"

static void not_implemented(void);

int construct_none_label(label_t *this)
{
	this->layout = LYT_NONE;
	
	this->add_part = add_none_part;
	this->delete_part = delete_none_part;
	this->destroy_label = destroy_none_label;
	this->new_label = new_none_label;
	this->print_parts = print_none_parts;
	this->read_parts = read_none_parts;
	this->write_parts = write_none_parts;
	this->extra_funcs = extra_none_funcs;
	
	return EOK;
}

int add_none_part(label_t *this, tinput_t *in)
{
	not_implemented();
	return EOK;
}

int delete_none_part(label_t *this, tinput_t *in)
{
	not_implemented();
	return EOK;
}

int destroy_none_label(label_t *this)
{
	return EOK;
}

int new_none_label(label_t *this)
{
	not_implemented();
	return EOK;
}

int print_none_parts(label_t *this)
{
	not_implemented();
	return EOK;
}

int read_none_parts(label_t *this)
{
	not_implemented();
	return EOK;
}

int write_none_parts(label_t *this)
{
	not_implemented();
	return EOK;
}

int extra_none_funcs(label_t *this, tinput_t * in)
{
	not_implemented();
	return EOK;
}

static void not_implemented(void)
{
	printf("No format selected.\n");
}
