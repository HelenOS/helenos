/*
 * SPDX-FileCopyrightText: 2011 Martin Sucha
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <io/console.h>
#include <errno.h>
#include <fmtutil.h>
#include <stdlib.h>
#include <str.h>

typedef struct {
	align_mode_t alignment;
	bool newline_always;
	size_t width;
} printmode_t;

errno_t print_wrapped_console(const char *str, align_mode_t alignment)
{
	console_ctrl_t *console = console_init(stdin, stdout);
	if (console == NULL) {
		printf("%s", str);
		return EOK;
	}
	sysarg_t con_rows, con_cols, con_col, con_row;
	errno_t rc = console_get_size(console, &con_cols, &con_rows);
	if (rc != EOK) {
		return rc;
	}
	rc = console_get_pos(console, &con_col, &con_row);
	if (rc != EOK) {
		return rc;
	}
	if (con_col != 0) {
		printf("\n");
	}
	return print_wrapped(str, con_cols, alignment);
}

/** Line consumer that prints the lines aligned according to spec */
static errno_t print_line(char32_t *wstr, size_t chars, bool last, void *data)
{
	printmode_t *pm = (printmode_t *) data;
	char32_t old_char = wstr[chars];
	wstr[chars] = 0;
	errno_t rc = print_aligned_w(wstr, pm->width, last, pm->alignment);
	wstr[chars] = old_char;
	return rc;
}

errno_t print_wrapped(const char *str, size_t width, align_mode_t mode)
{
	printmode_t pm;
	pm.alignment = mode;
	pm.newline_always = false;
	pm.width = width;
	char32_t *wstr = str_to_awstr(str);
	if (wstr == NULL) {
		return ENOMEM;
	}
	errno_t rc = wrap(wstr, width, print_line, &pm);
	free(wstr);
	return rc;
}

errno_t print_aligned_w(const char32_t *wstr, size_t width, bool last,
    align_mode_t mode)
{
	size_t i;
	size_t len = wstr_length(wstr);
	if (mode == ALIGN_LEFT || (mode == ALIGN_JUSTIFY && last)) {
		for (i = 0; i < width; i++) {
			if (i < len)
				putuchar(wstr[i]);
			else
				putuchar(' ');
		}
	} else if (mode == ALIGN_RIGHT) {
		for (i = 0; i < width; i++) {
			if (i < width - len)
				putuchar(' ');
			else
				putuchar(wstr[i - (width - len)]);
		}
	} else if (mode == ALIGN_CENTER) {
		size_t padding = (width - len) / 2;
		for (i = 0; i < width; i++) {
			if ((i < padding) || ((i - padding) >= len))
				putuchar(' ');
			else
				putuchar(wstr[i - padding]);
		}
	} else if (mode == ALIGN_JUSTIFY) {
		size_t words = 0;
		size_t word_chars = 0;
		bool space = true;
		for (i = 0; i < len; i++) {
			if (space && wstr[i] != ' ') {
				words++;
			}
			space = wstr[i] == ' ';
			if (!space)
				word_chars++;
		}
		size_t done_words = 0;
		size_t done_chars = 0;
		if (words == 0)
			goto skip_words;
		size_t excess_spaces = width - word_chars - (words - 1);
		space = true;
		i = 0;
		size_t j;
		while (i < len) {
			/* Find a word */
			while (i < len && wstr[i] == ' ')
				i++;
			if (i == len)
				break;
			if (done_words) {
				size_t spaces = 1 + (((done_words *
				    excess_spaces) / (words - 1)) -
				    (((done_words - 1) * excess_spaces) /
				    (words - 1)));
				for (j = 0; j < spaces; j++) {
					putuchar(' ');
				}
				done_chars += spaces;
			}
			while (i < len && wstr[i] != ' ') {
				putuchar(wstr[i++]);
				done_chars++;
			}
			done_words++;
		}
	skip_words:
		while (done_chars < width) {
			putuchar(' ');
			done_chars++;
		}
	} else {
		return EINVAL;
	}

	return EOK;
}
errno_t print_aligned(const char *str, size_t width, bool last, align_mode_t mode)
{
	char32_t *wstr = str_to_awstr(str);
	if (wstr == NULL) {
		return ENOMEM;
	}
	errno_t rc = print_aligned_w(wstr, width, last, mode);
	free(wstr);
	return rc;
}

errno_t wrap(char32_t *wstr, size_t width, line_consumer_fn consumer, void *data)
{
	size_t word_start = 0;
	size_t last_word_end = 0;
	size_t line_start = 0;
	size_t line_len = 0;
	size_t pos = 0;

	/*
	 * Invariants:
	 *  * line_len = last_word_end - line_start
	 *  * line_start <= last_word_end <= word_start <= pos
	 */

	while (wstr[pos] != 0) {
		/* Skip spaces and process newlines */
		while (wstr[pos] == ' ' || wstr[pos] == '\n') {
			if (wstr[pos] == '\n') {
				consumer(wstr + line_start, line_len, true,
				    data);
				last_word_end = line_start = pos + 1;
				line_len = 0;
			}
			pos++;
		}
		word_start = pos;
		/* Find end of word */
		while (wstr[pos] != 0 && wstr[pos] != ' ' &&
		    wstr[pos] != '\n')
			pos++;
		bool last = wstr[pos] == 0;
		/* Check if the line still fits width */
		if (pos - line_start > width) {
			if (line_len > 0)
				consumer(wstr + line_start, line_len, last,
				    data);
			line_start = last_word_end = word_start;
			line_len = 0;
		}
		/* Check if we need to force wrap of long word */
		if (pos - word_start > width) {
			consumer(wstr + word_start, width, last, data);
			pos = line_start = last_word_end = word_start + width;
			line_len = 0;
		}
		last_word_end = pos;
		line_len = last_word_end - line_start;
	}
	/*
	 * Here we have less than width chars starting from line_start.
	 * Moreover, the last portion does not contain spaces or newlines
	 */
	if (pos - line_start > 0)
		consumer(wstr + line_start, pos - line_start, true, data);

	return EOK;
}
