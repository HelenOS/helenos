/*
 * Copyright (c) 2016 Jiri Svoboda
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

/** @addtogroup kernel_generic_adt
 * @{
 */
/** @file
 */

#ifndef _LIBC_TYPES_ODICT_H_
#define _LIBC_TYPES_ODICT_H_

#include <adt/list.h>

typedef struct odlink odlink_t;
typedef struct odict odict_t;

typedef void *(*odgetkey_t)(odlink_t *);
typedef int (*odcmp_t)(void *, void *);

typedef enum {
	odc_black,
	odc_red
} odict_color_t;

typedef enum {
	/** Child A */
	odcs_a,
	/** Child B */
	odcs_b
} odict_child_sel_t;

/** Ordered dictionary link */
struct odlink {
	/** Containing dictionary */
	odict_t *odict;
	/** Parent node */
	odlink_t *up;
	/** First child */
	odlink_t *a;
	/** Second child */
	odlink_t *b;
	/** Node color */
	odict_color_t color;
	/** Link to odict->entries */
	link_t lentries;
};

/** Ordered dictionary */
struct odict {
	/** Root of the tree */
	odlink_t *root;
	/** List of entries in ascending order */
	list_t entries;
	/** Get key operation */
	odgetkey_t getkey;
	/** Compare operation */
	odcmp_t cmp;
};

#endif

/** @}
 */
