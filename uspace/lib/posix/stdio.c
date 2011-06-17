/*
 * Copyright (c) 2011 Jiri Zarevucky
 * Copyright (c) 2011 Petr Koupy
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

/** @addtogroup libposix
 * @{
 */
/** @file
 */

#define LIBPOSIX_INTERNAL

#include <assert.h>
#include <errno.h>

#include "common.h"
#include "stdio.h"
#include "string.h"

/* not the best of solutions, but freopen will eventually
 * need to be implemented in libc anyway
 */
#include "../c/generic/private/stdio.h"

/**
 * 
 * @param c
 * @param stream
 * @return 
 */
int posix_ungetc(int c, FILE *stream)
{
	// TODO
	not_implemented();
}

/**
 * 
 * @param filename
 * @param mode
 * @param stream
 * @return
 */
FILE *posix_freopen(
    const char *restrict filename,
    const char *restrict mode,
    FILE *restrict stream)
{
	assert(mode != NULL);
	assert(stream != NULL);

	if (filename == NULL) {
		// TODO
		
		/* print error to stderr as well, to avoid hard to find problems
		 * with buggy apps that expect this to work
		 */
		fprintf(stderr,
		    "ERROR: Application wants to use freopen() to change mode of opened stream.\n"
		    "       libposix does not support that yet, the application may function improperly.\n");
		errno = ENOTSUP;
		return NULL;
	}

	FILE* copy = malloc(sizeof(FILE));
	if (copy == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	memcpy(copy, stream, sizeof(FILE));
	fclose(copy); /* copy is now freed */
	
	copy = fopen(filename, mode); /* open new stream */
	if (copy == NULL) {
		/* fopen() sets errno */
		return NULL;
	}
	
	/* move the new stream to the original location */
	memcpy(stream, copy, sizeof (FILE));
	free(copy);
	
	/* update references in the file list */
	stream->link.next->prev = &stream->link;
	stream->link.prev->next = &stream->link;
	
	return stream;
}

/**
 *
 * @param s
 */
void posix_perror(const char *s)
{
	// TODO
	not_implemented();
}

/**
 * 
 * @param stream
 * @param offset
 * @param whence
 * @return
 */
int posix_fseeko(FILE *stream, posix_off_t offset, int whence)
{
	// TODO
	not_implemented();
}

/**
 * 
 * @param stream
 * @return
 */
posix_off_t posix_ftello(FILE *stream)
{
	// TODO
	not_implemented();
}

/**
 * 
 * @param s
 * @param format
 * @param ...
 * @return
 */
int posix_sprintf(char *s, const char *format, ...)
{
	// TODO
	not_implemented();
}

/**
 * 
 * @param s
 * @param format
 * @param ...
 * @return
 */
int posix_sscanf(const char *s, const char *format, ...)
{
	// TODO
	not_implemented();
}

/** @}
 */
