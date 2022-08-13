/*
 * SPDX-FileCopyrightText: 2012 Sandeep Kumar
 * SPDX-FileCopyrightText: 2012 Vojtech Horky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
