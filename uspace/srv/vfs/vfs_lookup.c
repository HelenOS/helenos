/*
 * Copyright (c) 2008 Jakub Jermar
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

/** @addtogroup fs
 * @{
 */ 

/**
 * @file	vfs_lookup.c
 * @brief
 */

#include <ipc/ipc.h>
#include <async.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <bool.h>
#include <futex.h>
#include <libadt/list.h>
#include <atomic.h>
#include "vfs.h"

#define min(a, b)	((a) < (b) ? (a) : (b))

/* Forward static declarations. */
static char *canonify(char *path);

atomic_t plb_futex = FUTEX_INITIALIZER;
link_t plb_head;	/**< PLB entry ring buffer. */
uint8_t *plb = NULL;

/** Perform a path lookup.
 *
 * @param path		Path to be resolved; it needn't be an ASCIIZ string.
 * @param len		Number of path characters pointed by path.
 * @param lflag		Flags to be used during lookup.
 * @param result	Empty structure where the lookup result will be stored.
 *			Can be NULL.
 * @param altroot	If non-empty, will be used instead of rootfs as the root
 *			of the whole VFS tree.
 *
 * @return		EOK on success or an error code from errno.h.
 */
int vfs_lookup_internal(char *path, size_t len, int lflag,
    vfs_lookup_res_t *result, vfs_pair_t *altroot)
{
	vfs_pair_t *root;

	if (!len)
		return EINVAL;

	if (altroot)
		root = altroot;
	else
		root = (vfs_pair_t *) &rootfs;

	if (!root->fs_handle)
		return ENOENT;
	
	futex_down(&plb_futex);

	plb_entry_t entry;
	link_initialize(&entry.plb_link);
	entry.len = len;

	off_t first;	/* the first free index */
	off_t last;	/* the last free index */

	if (list_empty(&plb_head)) {
		first = 0;
		last = PLB_SIZE - 1;
	} else {
		plb_entry_t *oldest = list_get_instance(plb_head.next,
		    plb_entry_t, plb_link);
		plb_entry_t *newest = list_get_instance(plb_head.prev,
		    plb_entry_t, plb_link);

		first = (newest->index + newest->len) % PLB_SIZE;
		last = (oldest->index - 1) % PLB_SIZE;
	}

	if (first <= last) {
		if ((last - first) + 1 < len) {
			/*
			 * The buffer cannot absorb the path.
			 */
			futex_up(&plb_futex);
			return ELIMIT;
		}
	} else {
		if (PLB_SIZE - ((first - last) + 1) < len) {
			/*
			 * The buffer cannot absorb the path.
			 */
			futex_up(&plb_futex);
			return ELIMIT;
		}
	}

	/*
	 * We know the first free index in PLB and we also know that there is
	 * enough space in the buffer to hold our path.
	 */

	entry.index = first;
	entry.len = len;

	/*
	 * Claim PLB space by inserting the entry into the PLB entry ring
	 * buffer.
	 */
	list_append(&entry.plb_link, &plb_head);
	
	futex_up(&plb_futex);

	/*
	 * Copy the path into PLB.
	 */
	size_t cnt1 = min(len, (PLB_SIZE - first) + 1);
	size_t cnt2 = len - cnt1;
	
	memcpy(&plb[first], path, cnt1);
	memcpy(plb, &path[cnt1], cnt2);

	ipc_call_t answer;
	int phone = vfs_grab_phone(root->fs_handle);
	aid_t req = async_send_4(phone, VFS_LOOKUP, (ipcarg_t) first,
	    (ipcarg_t) (first + len - 1) % PLB_SIZE,
	    (ipcarg_t) root->dev_handle, (ipcarg_t) lflag, &answer);
	vfs_release_phone(phone);

	ipcarg_t rc;
	async_wait_for(req, &rc);

	futex_down(&plb_futex);
	list_remove(&entry.plb_link);
	/*
	 * Erasing the path from PLB will come handy for debugging purposes.
	 */
	memset(&plb[first], 0, cnt1);
	memset(plb, 0, cnt2);
	futex_up(&plb_futex);

	if ((rc == EOK) && result) {
		result->triplet.fs_handle = (int) IPC_GET_ARG1(answer);
		result->triplet.dev_handle = (int) IPC_GET_ARG2(answer);
		result->triplet.index = (int) IPC_GET_ARG3(answer);
		result->size = (size_t) IPC_GET_ARG4(answer);
		result->lnkcnt = (unsigned) IPC_GET_ARG5(answer);
	}

	return rc;
}

/** Token types used for tokenization of path. */
typedef enum {
	TK_INVALID,
	TK_SLASH,
	TK_DOT,
	TK_DOTDOT,
	TK_COMP,
	TK_NUL
} tokval_t;

typedef struct {
	tokval_t kind;
	char *start;
	char *stop;
} token_t;

/** Fake up the TK_SLASH token. */
static token_t slash_token(char *start)
{
	token_t ret;
	ret.kind = TK_SLASH;
	ret.start = start;
	ret.stop = start;
	return ret;
}

/** Given a token, return the next token. */
static token_t next_token(token_t *cur)
{
	token_t ret;

	if (cur->stop[1] == '\0') {
		ret.kind = TK_NUL;
		ret.start = cur->stop + 1;
		ret.stop = ret.start;
		return ret;
	}
	if (cur->stop[1] == '/') {
		ret.kind = TK_SLASH;
		ret.start = cur->stop + 1;
		ret.stop = ret.start;
		return ret;
	}
	if (cur->stop[1] == '.' && (!cur->stop[2] || cur->stop[2] == '/')) {
		ret.kind = TK_DOT;
		ret.start = cur->stop + 1;
		ret.stop = ret.start;
		return ret;
	}
	if (cur->stop[1] == '.' && cur->stop[2] == '.' &&
	    (!cur->stop[3] || cur->stop[3] == '/')) {
		ret.kind = TK_DOTDOT;
		ret.start = cur->stop + 1;
		ret.stop = cur->stop + 2;
		return ret;
	}
	unsigned i;
	for (i = 1; cur->stop[i] && cur->stop[i] != '/'; i++)
		;
	ret.kind = TK_COMP;
	ret.start = &cur->stop[1];
	ret.stop = &cur->stop[i - 1];
	return ret;
}

/** States used by canonify(). */
typedef enum {
	S_INI,
	S_A,
	S_B,
	S_C,
	S_ACCEPT,
	S_RESTART,
	S_REJECT
} state_t;

typedef struct {
	state_t s;
	void (* f)(token_t *, token_t *, token_t *);
} change_state_t;

/*
 * Actions that can be performed when transitioning from one
 * state of canonify() to another.
 */
static void set_first_slash(token_t *t, token_t *tfsl, token_t *tlcomp)
{
	*tfsl = *t;
}
static void save_component(token_t *t, token_t *tfsl, token_t *tlcomp)
{
	*tlcomp = *t;
}
static void terminate_slash(token_t *t, token_t *tfsl, token_t *tlcomp)
{
	tfsl->stop[1] = '\0';
}
static void remove_trailing_slash(token_t *t, token_t *tfsl, token_t *tlcomp)
{
	t->start[-1] = '\0';
}
/** Eat the extra '/'..
 *
 * @param t		The current TK_SLASH token.
 */
static void shift_slash(token_t *t, token_t *tfsl, token_t *tlcomp)
{
	char *p = t->start;
	char *q = t->stop + 1;
	while ((*p++ = *q++))
		;
}
/** Eat the extra '.'.
 *
 * @param t		The current TK_DOT token.
 */
static void shift_dot(token_t *t, token_t *tfsl, token_t *tlcomp)
{
	char *p = t->start;
	char *q = t->stop + 1;
	while ((*p++ = *q++))
		;
}
/** Collapse the TK_COMP TK_SLASH TK_DOTDOT pattern.
 *
 * @param t		The current TK_DOTDOT token.
 * @param tlcomp	The last TK_COMP token.
 */
static void shift_dotdot(token_t *t, token_t *tfsl, token_t *tlcomp)
{
	char *p = tlcomp->start;
	char *q = t->stop + 1;
	while ((*p++ = *q++))
		;
}

/** Transition function for canonify(). */
static change_state_t trans[4][6] = {
	[S_INI] = {
		[TK_SLASH] = {
			.s = S_A,
			.f = set_first_slash,
		},
		[TK_DOT] = {
			.s = S_REJECT,
			.f = NULL,
		},
		[TK_DOTDOT] = {
			.s = S_REJECT,
			.f = NULL,
		},
		[TK_COMP] = {
			.s = S_REJECT,
			.f = NULL,
		},
		[TK_NUL] = {
			.s = S_REJECT,
			.f = NULL,
		},
		[TK_INVALID] = {
			.s = S_REJECT,
			.f = NULL,
		},
	},
	[S_A] = {
		[TK_SLASH] = {
			.s = S_A,
			.f = set_first_slash,
		},
		[TK_DOT] = {
			.s = S_A,
			.f = NULL,
		},
		[TK_DOTDOT] = {
			.s = S_A,
			.f = NULL,
		},
		[TK_COMP] = {
			.s = S_B,
			.f = save_component,
		},
		[TK_NUL] = {
			.s = S_ACCEPT,
			.f = terminate_slash,
		},
		[TK_INVALID] = {
			.s = S_REJECT,
			.f = NULL,
		},
	},
	[S_B] = {
		[TK_SLASH] = {
			.s = S_C,
			.f = NULL,
		},
		[TK_DOT] = {
			.s = S_REJECT,
			.f = NULL,
		},
		[TK_DOTDOT] = {
			.s = S_REJECT,
			.f = NULL,
		},
		[TK_COMP] = {
			.s = S_REJECT,
			.f = NULL,
		},
		[TK_NUL] = {
			.s = S_ACCEPT,
			.f = NULL,
		},
		[TK_INVALID] = {
			.s = S_REJECT,
			.f = NULL,
		},
	},
	[S_C] = {
		[TK_SLASH] = {
			.s = S_RESTART,
			.f = shift_slash,
		},
		[TK_DOT] = {
			.s = S_RESTART,
			.f = shift_dot,
		},
		[TK_DOTDOT] = {
			.s = S_RESTART,
			.f = shift_dotdot,
		},
		[TK_COMP] = {
			.s = S_B,
			.f = save_component,
		},
		[TK_NUL] = {
			.s = S_ACCEPT,
			.f = remove_trailing_slash,
		},
		[TK_INVALID] = {
			.s = S_REJECT,
			.f = NULL,
		},
	}
};

/** Canonify a file system path.
 *
 * A file system path is canonical, if the following holds:
 * 1) the path is absolute (i.e. a/b/c is not canonical)
 * 2) there is no trailing slash in the path (i.e. /a/b/c is not canonical)
 * 3) there is no extra slash in the path (i.e. /a//b/c is not canonical)
 * 4) there is no '.' component in the path (i.e. /a/./b/c is not canonical)
 * 5) there is no '..' component in the path (i.e. /a/b/../c is not canonical) 
 *
 * This function makes a potentially non-canonical file system path canonical.
 * It works in-place and requires a NULL-terminated input string.
 *
 * @param		Path to be canonified.
 *
 * @return		Canonified path or NULL on failure.
 */
char *canonify(char *path)
{
	state_t state;
	token_t t;
	token_t tfsl;		/* first slash */
	token_t tlcomp;		/* last component */
	if (*path != '/')
		return NULL;
	tfsl = slash_token(path);
restart:
	state = S_INI;
	t = tfsl;
	while (state != S_ACCEPT && state != S_RESTART && state != S_REJECT) {
		if (trans[state][t.kind].f)
			trans[state][t.kind].f(&t, &tfsl, &tlcomp);
		state = trans[state][t.kind].s;
		t = next_token(&t);
	}
	
	switch (state) {
	case S_RESTART:
		goto restart;
	case S_REJECT:
		return NULL;
	case S_ACCEPT:
		return tfsl.start; 
	default:
		abort();
	}
}

/**
 * @}
 */
