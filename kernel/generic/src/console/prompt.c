/*
 * Copyright (c) 2012 Sandeep Kumar
 * Copyright (c) 2012 Vojtech Horky
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

/** @addtogroup kernel_generic_console
 * @{
 */

/**
 * @file
 * @brief Kernel console special prompts.
 */

#include <assert.h>
#include <console/prompt.h>

/** Display the <i>display all possibilities</i> prompt and wait for answer.
 *
 * @param indev Where to read characters from.
 * @param hints Number of hints that would be displayed.
 *
 * @return Whether to print all hints.
 *
 */
bool console_prompt_display_all_hints(indev_t *indev, size_t hints)
{
	assert(indev);
	assert(hints > 0);

	printf("Display all %zu possibilities? (y or n) ", hints);

	while (true) {
		char32_t answer = indev_pop_character(indev);

		if ((answer == 'y') || (answer == 'Y')) {
			printf("y");
			return true;
		}

		if ((answer == 'n') || (answer == 'N')) {
			printf("n");
			return false;
		}
	}
}

/** Display the <i>--more--</i> prompt and wait for answer.
 *
 * When the function returns false, @p display_hints is set to zero.
 *
 * @param[in]  indev         Where to read characters from.
 * @param[out] display_hints How many hints to display.
 *
 * @return Whether to display more hints.
 *
 */
bool console_prompt_more_hints(indev_t *indev, size_t *display_hints)
{
	assert(indev);
	assert(display_hints != NULL);

	printf("--More--");
	while (true) {
		char32_t continue_showing_hints = indev_pop_character(indev);
		/* Display a full page again? */
		if ((continue_showing_hints == 'y') ||
		    (continue_showing_hints == 'Y') ||
		    (continue_showing_hints == ' ')) {
			*display_hints = MAX_TAB_HINTS - 1;
			break;
		}

		/* Stop displaying hints? */
		if ((continue_showing_hints == 'n') ||
		    (continue_showing_hints == 'N') ||
		    (continue_showing_hints == 'q') ||
		    (continue_showing_hints == 'Q')) {
			*display_hints = 0;
			break;
		}

		/* Show one more hint? */
		if (continue_showing_hints == '\n') {
			*display_hints = 1;
			break;
		}
	}

	/* Delete the --More-- option */
	printf("\r         \r");

	return *display_hints > 0;
}

/** @}
 */
