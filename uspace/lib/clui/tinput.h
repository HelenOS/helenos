/*
 * SPDX-FileCopyrightText: 2011 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libclui
 * @{
 */
/**
 * @file
 */

#ifndef LIBCLUI_TINPUT_H_
#define LIBCLUI_TINPUT_H_

#include <inttypes.h>
#include <io/console.h>
#include <stdio.h>

#define HISTORY_LEN     10
#define INPUT_MAX_SIZE  1024

/** Begin enumeration of text completions.
 *
 * When user requests text completion, tinput will call this function to start
 * text completion operation. @a *cstart should be set to the position
 * (character index) of the first character of the 'word' that is being
 * completed. The resulting text is obtained by replacing the range of text
 * starting at @a *cstart and ending at @a pos with the completion text.
 *
 * The function can pass information to the get_next and fini functions
 * via @a state. The init function allocates the state object and stores
 * a pointer to it to @a *state. The fini function destroys the state object.
 *
 * @param text		Current contents of edit buffer (null-terminated).
 * @param pos		Current caret position.
 * @param cstart	Output, position in text where completion begins from.
 * @param state		Output, pointer to a client state object.
 *
 * @return		EOK on success, error code on failure.
 */
typedef errno_t (*tinput_compl_init_fn)(char32_t *text, size_t pos, size_t *cstart,
    void **state);

/** Obtain one text completion alternative.
 *
 * Upon success the function sets @a *compl to point to a string, the
 * completion text. The caller (Tinput) should not modify or free the text.
 * The pointer is only valid until the next invocation of any completion
 * function.
 *
 * @param state		Pointer to state object created by the init funtion.
 * @param compl		Output, the completion text, ownership retained.
 *
 * @return		EOK on success, error code on failure.
 */
typedef errno_t (*tinput_compl_get_next_fn)(void *state, char **compl);

/** Finish enumeration of text completions.
 *
 * The function must deallocate any state information allocated by the init
 * function or temporary data allocated by the get_next function.
 *
 * @param state		Pointer to state object created by the init funtion.
 */
typedef void (*tinput_compl_fini_fn)(void *state);

/** Text completion ops. */
typedef struct {
	tinput_compl_init_fn init;
	tinput_compl_get_next_fn get_next;
	tinput_compl_fini_fn fini;
} tinput_compl_ops_t;

/** Text input field (command line).
 *
 * Applications should treat this structure as opaque.
 */
typedef struct {
	/** Console */
	console_ctrl_t *console;

	/** Prompt string */
	char *prompt;

	/** Completion ops. */
	tinput_compl_ops_t *compl_ops;

	/** Buffer holding text currently being edited */
	char32_t buffer[INPUT_MAX_SIZE + 1];

	/** Linear position on screen where the prompt starts */
	unsigned prompt_coord;
	/** Linear position on screen where the text field starts */
	unsigned text_coord;

	/** Screen dimensions */
	sysarg_t con_cols;
	sysarg_t con_rows;

	/** Number of characters in @c buffer */
	size_t nc;

	/** Caret position within buffer */
	size_t pos;

	/** Selection mark position within buffer */
	size_t sel_start;

	/** History (dynamically allocated strings) */
	char *history[HISTORY_LEN + 1];

	/** Number of entries in @c history, not counting [0] */
	size_t hnum;

	/** Current position in history */
	size_t hpos;

	/** @c true if finished with this line (return to caller) */
	bool done;

	/** @c true if user requested to abort interactive loop */
	bool exit_clui;

	/** @c true if left shift key is currently held */
	bool lshift_held;

	/** @c true if right shift key is currently held */
	bool rshift_held;
} tinput_t;

extern tinput_t *tinput_new(void);
extern errno_t tinput_set_prompt(tinput_t *, const char *);
extern void tinput_set_compl_ops(tinput_t *, tinput_compl_ops_t *);
extern void tinput_destroy(tinput_t *);
extern errno_t tinput_read(tinput_t *, char **);
extern errno_t tinput_read_i(tinput_t *, const char *, char **);

#endif

/** @}
 */
