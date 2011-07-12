/*
 * Copyright (c) 2009 Lukas Mejdrech
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

/** @addtogroup libc
 *  @{
 */

/** @file
 * Character string with measured length.
 * The structure has been designed for serialization of character strings
 * between modules.
 */

#ifndef LIBC_MEASURED_STRINGS_H_
#define LIBC_MEASURED_STRINGS_H_

#include <sys/types.h>
#include <async.h>

/** Type definition of the character string with measured length.
 * @see measured_string
 */
typedef struct measured_string measured_string_t;

/** Character string with measured length.
 *
 * This structure has been designed for serialization of character strings
 * between modules.
 */
struct measured_string {
	/** Character string data. */
	uint8_t *value;
	/** Character string length. */
	size_t length;
};

extern measured_string_t *measured_string_create_bulk(const uint8_t *, size_t);
extern measured_string_t *measured_string_copy(measured_string_t *);

extern int measured_strings_receive(measured_string_t **, uint8_t **, size_t);
extern int measured_strings_reply(const measured_string_t *, size_t);

extern int measured_strings_return(async_exch_t *, measured_string_t **,
    uint8_t **, size_t);
extern int measured_strings_send(async_exch_t *, const measured_string_t *,
    size_t);

#endif

/** @}
 */
