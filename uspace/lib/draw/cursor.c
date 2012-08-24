/*
 * Copyright (c) 2012 Petr Koupy
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

/** @addtogroup draw
 * @{
 */
/**
 * @file
 */

#include <malloc.h>

#include "cursor.h"
#include "cursor/embedded.h"

void cursor_init(cursor_t *cursor, cursor_decoder_type_t decoder, char *path)
{
	switch (decoder) {
	case CURSOR_DECODER_EMBEDDED:
		cursor->decoder = &cd_embedded;
		break;
	default:
		cursor->decoder = NULL;
		break;
	}

	if (cursor->decoder) {
		cursor->decoder->init(path, &cursor->state_count, &cursor->decoder_data);

		if (cursor->state_count > 0) {
			cursor->states = (surface_t **) malloc(sizeof(surface_t *) * cursor->state_count);
		} else {
			cursor->states = NULL;
		}

		if (cursor->states) {
			for (size_t i = 0; i < cursor->state_count; ++i) {
				cursor->states[i] = cursor->decoder->render(i);
			}
		} else {
			cursor->state_count = 0;
		}
	} else {
		cursor->state_count = 0;
		cursor->states = NULL;
	}
}

void cursor_release(cursor_t *cursor)
{
	if (cursor->states) {
		for (size_t i = 0; i < cursor->state_count; ++i) {
			if (cursor->states[i]) {
				surface_destroy(cursor->states[i]);
			}
		}
		free(cursor->states);
	}

	if (cursor->decoder) {
		cursor->decoder->release(cursor->decoder_data);
	}
}

/** @}
 */
