/*
 * SPDX-FileCopyrightText: 2011 Martin Sucha
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
