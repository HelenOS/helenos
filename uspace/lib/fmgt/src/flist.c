/*
 * Copyright (c) 2025 Jiri Svoboda
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

/** @addtogroup fmgt
 * @{
 */
/** @file File list.
 */

#include <adt/list.h>
#include <errno.h>
#include <stdlib.h>
#include <str.h>

#include "fmgt.h"

/** Create file list.
 *
 * @param rflist Place to store pointer to new file list.
 * @return EOK on success, ENOMEM if out of memory.
 */
errno_t fmgt_flist_create(fmgt_flist_t **rflist)
{
	fmgt_flist_t *flist;

	flist = calloc(1, sizeof(fmgt_flist_t));
	if (flist == NULL)
		return ENOMEM;

	list_initialize(&flist->files);
	*rflist = flist;
	return EOK;
}

/** Append file name to file list.
 *
 * @param flist File list
 * @param fname File name to append
 * @return EOK on success, ENOMEM if out of memory
 */
errno_t fmgt_flist_append(fmgt_flist_t *flist, const char *fname)
{
	fmgt_flist_entry_t *entry;

	entry = calloc(1, sizeof(fmgt_flist_entry_t));
	if (entry == NULL)
		return ENOMEM;

	entry->fname = str_dup(fname);
	if (entry->fname == NULL) {
		free(entry);
		return ENOMEM;
	}

	entry->flist = flist;
	list_append(&entry->lfiles, &flist->files);

	return EOK;
}

/** Destroy file list.
 *
 * @param flist File list
 */
void fmgt_flist_destroy(fmgt_flist_t *flist)
{
	fmgt_flist_entry_t *entry;

	entry = fmgt_flist_first(flist);
	while (entry != NULL) {
		list_remove(&entry->lfiles);
		free(entry->fname);
		free(entry);
		entry = fmgt_flist_first(flist);
	}

	free(flist);
}

/** Get first file list entry.
 *
 * @param flist File list
 * @return First file list entry or @c NULL iff list list empty
 */
fmgt_flist_entry_t *fmgt_flist_first(fmgt_flist_t *flist)
{
	link_t *link;

	link = list_first(&flist->files);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, fmgt_flist_entry_t, lfiles);
}

/** Get first new list entry.
 *
 * @param entry Current file list entry
 * @return Next file list entry or @c NULL iff @a entry is last
 */
fmgt_flist_entry_t *fmgt_flist_next(fmgt_flist_entry_t *entry)
{
	link_t *link;

	link = list_next(&entry->lfiles, &entry->flist->files);
	if (link == NULL)
		return NULL;

	return list_get_instance(link, fmgt_flist_entry_t, lfiles);
}

/** Return number of entries in file list.
 *
 * @param flist File list
 * @return Number of entries in @a flist.
 */
unsigned fmgt_flist_count(fmgt_flist_t *flist)
{
	unsigned count;
	fmgt_flist_entry_t *entry;

	count = 0;
	entry = fmgt_flist_first(flist);
	while (entry != NULL) {
		++count;
		entry = fmgt_flist_next(entry);
	}

	return count;
}

/** @}
 */
