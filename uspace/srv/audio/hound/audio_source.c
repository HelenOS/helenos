/*
 * Copyright (c) 2012 Jan Vesely
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

/** @addtogroup audio
 * @brief HelenOS sound server
 * @{
 */
/** @file
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <str.h>
#include <str_error.h>

#include "audio_source.h"
#include "log.h"


int audio_source_init(audio_source_t *source, const char* name)
{
	assert(source);
	if (!name) {
		log_debug("Incorrect parameters.");
		return EINVAL;
	}
	link_initialize(&source->link);
	source->name = str_dup(name);
	source->connected_change.hook = NULL;
	source->connected_change.arg = NULL;
	source->get_data.hook = NULL;
	source->get_data.arg = NULL;
	source->available.base = NULL;
	source->available.size = 0;
	log_verbose("Initialized source (%p) '%s' with ANY audio format",
	    source, source->name);
	return EOK;
}

int audio_source_connected(audio_source_t *source, const audio_format_t *f)
{
	assert(source);
	if (!f) {
		log_debug("Incorrect parameters.");
		return EINVAL;
	}
	if (source->connected_change.hook)
		source->connected_change.hook(source->connected_change.arg, f);
	return EOK;
}

int audio_source_get_buffer(audio_source_t *source, const void **buffer, size_t *size)
{
	assert(source);
	if (!buffer || !size) {
		log_debug("Incorrect parameters.");
		return EINVAL;
	}
	if (source->get_data.hook)
		source->get_data.hook(source->get_data.arg);
	if (!source->available.base || !source->available.size)
		return EOVERFLOW; /* In fact it's underflow */

	const size_t requested_size =
	    (*size == 0) || (*size > source->available.size)
	    ? source->available.size : *size;
	*buffer = source->available.base;
	*size = requested_size;
	source->available.size -= requested_size;
	source->available.base += requested_size;
	return EOK;
}
/**
 * @}
 */
