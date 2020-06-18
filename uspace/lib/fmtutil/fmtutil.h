/*
 * Copyright (c) 2011 Martin Sucha
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

typedef enum {
	ALIGN_LEFT,
	ALIGN_RIGHT,
	ALIGN_CENTER,
	ALIGN_JUSTIFY
} align_mode_t;

/** Callback that processes a line of characters.
 * (e.g. as a result of wrap operation)
 *
 * @param content pointer to line data (note: this is NOT null-terminated)
 * @param size number of characters in line
 * @param end_of_para true if the line is the last line of the paragraph
 * @param data user data
 *
 * @returns EOK on success or an error code on failure
 */
typedef errno_t (*line_consumer_fn)(char32_t *, size_t, bool, void *);

extern errno_t print_aligned_w(const char32_t *, size_t, bool, align_mode_t);
extern errno_t print_aligned(const char *, size_t, bool, align_mode_t);
extern errno_t print_wrapped(const char *, size_t, align_mode_t);
extern errno_t print_wrapped_console(const char *, align_mode_t);

/** Wrap characters in a wide string to the given length.
 *
 * @param wstr the null-terminated wide string to wrap
 * @param size number of characters to wrap to
 * @param consumer the function that receives wrapped lines
 * @param data user data to pass to the consumer function
 */
extern errno_t wrap(char32_t *, size_t, line_consumer_fn, void *);
