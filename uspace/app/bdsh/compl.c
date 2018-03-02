/*
 * Copyright (c) 2011 Jiri Svoboda
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

#include <stdbool.h>
#include <dirent.h>
#include <errno.h>
#include <macros.h>
#include <stdlib.h>
#include <vfs/vfs.h>
#include <str.h>

#include "cmds/cmds.h"
#include "compl.h"
#include "exec.h"
#include "tok.h"

static errno_t compl_init(wchar_t *text, size_t pos, size_t *cstart, void **state);
static errno_t compl_get_next(void *state, char **compl);
static void compl_fini(void *state);

/** Bdsh implementation of completion ops. */
tinput_compl_ops_t compl_ops = {
	.init = compl_init,
	.get_next = compl_get_next,
	.fini = compl_fini
};

/** Completion state object.
 *
 * The state object contains 'iterators' for modules, builtins and
 * executables in directories.
 */
typedef struct {
	/** String prefix which we are trying to complete. */
	char *prefix;
	/** Length of string prefix (number of characters) */
	size_t prefix_len;

	/** Pointer inside list of modules */
	module_t *module;
	/** Pointer inside list of builtins */
	builtin_t *builtin;

	/** Pointer inside list of directories */
	const char *const *path;
	/** If not @c NULL, should be freed in the end. */
	char **path_list;
	/** Current open directory */
	DIR *dir;

	char *last_compl;

	/**
	 * @c true if we are completing a command, @c false if we are
	 * completing an argument
	 */
	bool is_command;
} compl_t;

/** Init completion.
 *
 * Set up iterators in completion object, based on current token.
 */
static errno_t compl_init(wchar_t *text, size_t pos, size_t *cstart, void **state)
{
	compl_t *cs = NULL;
	char *stext = NULL;
	char *prefix = NULL;
	char *dirname = NULL;
	errno_t retval;

	token_t *tokens = calloc(WORD_MAX, sizeof(token_t));
	if (tokens == NULL) {
		retval = ENOMEM;
		goto error;
	}

	size_t pref_size;
	char *rpath_sep;
	static const char *dirlist_arg[] = { ".", NULL };
	tokenizer_t tok;
	ssize_t current_token;
	size_t tokens_length;

	cs = calloc(1, sizeof(compl_t));
	if (!cs) {
		retval = ENOMEM;
		goto error;
	}

	/* Convert text buffer to string */
	stext = wstr_to_astr(text);
	if (stext == NULL) {
		retval = ENOMEM;
		goto error;
	}

	/* Tokenize the input string */
	retval = tok_init(&tok, stext, tokens, WORD_MAX);
	if (retval != EOK) {
		goto error;
	}

	retval = tok_tokenize(&tok, &tokens_length);
	if (retval != EOK) {
		goto error;
	}

	/* Find the current token */
	for (current_token = 0; current_token < (ssize_t) tokens_length;
	    current_token++) {
		token_t *t = &tokens[current_token];
		size_t end = t->char_start + t->char_length;

		/*
		 * Check if the caret lies inside the token or immediately
		 * after it
		 */
		if (t->char_start <= pos && pos <= end) {
			break;
		}
	}

	if (tokens_length == 0)
		current_token = -1;

	if ((current_token >= 0) && (tokens[current_token].type != TOKTYPE_SPACE))
		*cstart = tokens[current_token].char_start;
	else
		*cstart = pos;

	/*
	 * Extract the prefix being completed
	 * XXX: handle strings, etc.
	 */
	pref_size = str_lsize(stext, pos - *cstart);
	prefix = malloc(pref_size + 1);
	if (prefix == NULL) {
		retval = ENOMEM;
		goto error;
	}
	prefix[pref_size] = 0;

	if (current_token >= 0) {
		str_ncpy(prefix, pref_size + 1, stext +
		    tokens[current_token].byte_start, pref_size);
	}

	/*
	 * Determine if the token being completed is a command or argument.
	 * We look at the previous token. If there is none or it is a pipe
	 * ('|'), it is a command, otherwise it is an argument.
	 */

	/* Skip any whitespace before current token */
	ssize_t prev_token = current_token - 1;
	if ((prev_token >= 0) && (tokens[prev_token].type == TOKTYPE_SPACE))
		prev_token--;

	/*
	 * It is a command if it is the first token or if it immediately
	 * follows a pipe token.
	 */
	if ((prev_token < 0) || (tokens[prev_token].type == TOKTYPE_SPACE))
		cs->is_command = true;
	else
		cs->is_command = false;

	rpath_sep = str_rchr(prefix, '/');
	if (rpath_sep != NULL) {
		/* Extract path. For path beginning with '/' keep the '/'. */
		dirname = str_ndup(prefix, max(1, rpath_sep - prefix));
		if (dirname == NULL) {
			retval = ENOMEM;
			goto error;
		}

		/* Extract name prefix */
		cs->prefix = str_dup(rpath_sep + 1);
		if (cs->prefix == NULL) {
			retval = ENOMEM;
			goto error;
		}
		*cstart += rpath_sep + 1 - prefix;
		free(prefix);
		prefix = NULL;

		cs->path_list = malloc(sizeof(char *) * 2);
		if (cs->path_list == NULL) {
			retval = ENOMEM;
			goto error;
		}
		cs->path_list[0] = dirname;
		cs->path_list[1] = NULL;
		/* The second const ensures that we can't assign a const
		 * string to the non-const array. */
		cs->path = (const char *const *) cs->path_list;

	} else if (cs->is_command) {
		/* Command without path */
		cs->module = modules;
		cs->builtin = builtins;
		cs->prefix = prefix;
		cs->path = &search_dir[0];
	} else {
		/* Argument without path */
		cs->prefix = prefix;
		cs->path = &dirlist_arg[0];
	}

	cs->prefix_len = str_length(cs->prefix);

	tok_fini(&tok);

	*state = cs;
	return EOK;

error:
	/* Error cleanup */

	tok_fini(&tok);

	if (cs != NULL && cs->path_list != NULL) {
		size_t i = 0;
		while (cs->path_list[i] != NULL) {
			free(cs->path_list[i]);
			++i;
		}
		free(cs->path_list);
	}

	if ((cs != NULL) && (cs->prefix != NULL))
		free(cs->prefix);

	if (dirname != NULL)
		free(dirname);

	if (prefix != NULL)
		free(prefix);

	if (stext != NULL)
		free(stext);

	if (cs != NULL)
		free(cs);

	if (tokens != NULL)
		free(tokens);

	return retval;
}

/** Determine if completion matches the required prefix.
 *
 * Required prefix is stored in @a cs->prefix.
 *
 * @param cs	Completion state object
 * @param compl	Completion string
 * @return	@c true when @a compl matches, @c false otherwise
 */
static bool compl_match_prefix(compl_t *cs, const char *compl)
{
	return str_lcmp(compl, cs->prefix, cs->prefix_len) == 0;
}

/** Get next match. */
static errno_t compl_get_next(void *state, char **compl)
{
	compl_t *cs = (compl_t *) state;
	struct dirent *dent;

	*compl = NULL;

	if (cs->last_compl != NULL) {
		free(cs->last_compl);
		cs->last_compl = NULL;
	}

	/* Modules */
	if (cs->module != NULL) {
		while (*compl == NULL && cs->module->name != NULL) {
			if (compl_match_prefix(cs, cs->module->name)) {
				asprintf(compl, "%s ", cs->module->name);
				cs->last_compl = *compl;
				if (*compl == NULL)
					return ENOMEM;
			}
			cs->module++;
		}
	}

	/* Builtins */
	if (cs->builtin != NULL) {
		while (*compl == NULL && cs->builtin->name != NULL) {
			if (compl_match_prefix(cs, cs->builtin->name)) {
				asprintf(compl, "%s ", cs->builtin->name);
				cs->last_compl = *compl;
				if (*compl == NULL)
					return ENOMEM;
			}
			cs->builtin++;
		}
	}

	/* Files and directories. We scan entries from a set of directories. */
	if (cs->path != NULL) {
		while (*compl == NULL) {
			/* Open next directory */
			while (cs->dir == NULL) {
				if (*cs->path == NULL)
					break;

				cs->dir = opendir(*cs->path);

				/* Skip directories that we fail to open. */
				if (cs->dir == NULL)
					cs->path++;
			}

			/* If it was the last one, we are done */
			if (cs->dir == NULL)
				break;

			/* Read next directory entry */
			dent = readdir(cs->dir);
			if (dent == NULL) {
				/* Error. Close directory, go to next one */
				closedir(cs->dir);
				cs->dir = NULL;
				cs->path++;
				continue;
			}

			if (compl_match_prefix(cs, dent->d_name)) {
				/* Construct pathname */
				char *ent_path;
				asprintf(&ent_path, "%s/%s", *cs->path, dent->d_name);
				vfs_stat_t ent_stat;
				if (vfs_stat_path(ent_path, &ent_stat) != EOK) {
					/* Error */
					free(ent_path);
					continue;
				}

				free(ent_path);

				/* If completing command, do not match directories. */
				if (!ent_stat.is_directory || !cs->is_command) {
					asprintf(compl, "%s%c", dent->d_name,
					    ent_stat.is_directory ? '/' : ' ');
					cs->last_compl = *compl;
					if (*compl == NULL)
						return ENOMEM;
				}
			}
		}
	}

	if (*compl == NULL)
		return ENOENT;

	return EOK;
}

/** Finish completion operation. */
static void compl_fini(void *state)
{
	compl_t *cs = (compl_t *) state;

	if (cs->path_list != NULL) {
		size_t i = 0;
		while (cs->path_list[i] != NULL) {
			free(cs->path_list[i]);
			++i;
		}
		free(cs->path_list);
	}

	if (cs->last_compl != NULL)
		free(cs->last_compl);
	if (cs->dir != NULL)
		closedir(cs->dir);

	free(cs->prefix);
	free(cs);
}
