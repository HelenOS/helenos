/*
 * Copyright (c) 2009 Jiri Svoboda
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

/** @addtogroup edit
 * @{
 */
/**
 * @file
 * @brief Prototype implementation of Sheet data structure.
 *
 * The sheet is an abstract data structure representing a piece of text.
 * On top of this data structure we can implement a text editor. It is
 * possible to implement the sheet such that the editor can make small
 * changes to large files or files containing long lines efficiently.
 *
 * The sheet structure allows basic operations of text insertion, deletion,
 * retrieval and mapping of coordinates to position in the file and vice
 * versa. The text that is inserted or deleted can contain tabs and newlines
 * which are interpreted and properly acted upon.
 *
 * This is a trivial implementation with poor efficiency with O(N+n)
 * insertion and deletion and O(N) mapping (in both directions), where
 * N is the size of the file and n is the size of the inserted/deleted text.
 */

#include <stdlib.h>
#include <str.h>
#include <errno.h>
#include <adt/list.h>
#include <align.h>
#include <macros.h>

#include "sheet.h"
#include "sheet_impl.h"

enum {
	TAB_WIDTH	= 8,

	/** Initial  of dat buffer in bytes */
	INITIAL_SIZE	= 32
};

/** Initialize an empty sheet. */
errno_t sheet_create(sheet_t **rsh)
{
	sheet_t *sh;

	sh = calloc(1, sizeof(sheet_t));
	if (sh == NULL)
		return ENOMEM;

	sh->dbuf_size = INITIAL_SIZE;
	sh->text_size = 0;

	sh->data = malloc(sh->dbuf_size);
	if (sh->data == NULL)
		return ENOMEM;

	list_initialize(&sh->tags);

	*rsh = sh;
	return EOK;
}

/** Insert text into sheet.
 *
 * @param sh	Sheet to insert to.
 * @param pos	Point where to insert.
 * @param dir	Whether to insert before or after the point (affects tags).
 * @param str	The text to insert (printable characters, tabs, newlines).
 *
 * @return	EOK on success or an error code.
 *
 * @note	@a dir affects which way tags that were placed on @a pos will
 * 		move. If @a dir is @c dir_before, the tags will move forward
 *		and vice versa.
 */
errno_t sheet_insert(sheet_t *sh, spt_t *pos, enum dir_spec dir, char *str)
{
	char *ipp;
	size_t sz;
	char *newp;

	sz = str_size(str);
	if (sh->text_size + sz > sh->dbuf_size) {
		/* Enlarge data buffer. */
		newp = realloc(sh->data, sh->dbuf_size * 2);
		if (newp == NULL)
			return ELIMIT;

		sh->data = newp;
		sh->dbuf_size = sh->dbuf_size * 2;
	}

	ipp = sh->data + pos->b_off;

	/* Copy data. */
	memmove(ipp + sz, ipp, sh->text_size - pos->b_off);
	memcpy(ipp, str, sz);
	sh->text_size += sz;

	/* Adjust tags. */

	list_foreach(sh->tags, link, tag_t, tag) {
		if (tag->b_off > pos->b_off)
			tag->b_off += sz;
		else if (tag->b_off == pos->b_off && dir == dir_before)
			tag->b_off += sz;
	}

	return EOK;
}

/** Delete text from sheet.
 *
 * Deletes the range of text between two points from the sheet.
 *
 * @param sh	Sheet to delete from.
 * @param spos	Starting point.
 * @param epos	Ending point.
 *
 * @return	EOK on success or an error code.
 **/
errno_t sheet_delete(sheet_t *sh, spt_t *spos, spt_t *epos)
{
	char *spp;
	size_t sz;
	char *newp;
	size_t shrink_size;

	spp = sh->data + spos->b_off;
	sz = epos->b_off - spos->b_off;

	memmove(spp, spp + sz, sh->text_size - (spos->b_off + sz));
	sh->text_size -= sz;

	/* Adjust tags. */
	list_foreach(sh->tags, link, tag_t, tag) {
		if (tag->b_off >= epos->b_off)
			tag->b_off -= sz;
		else if (tag->b_off >= spos->b_off)
			tag->b_off = spos->b_off;
	}

	/* See if we should free up some memory. */
	shrink_size = max(sh->dbuf_size / 4, INITIAL_SIZE);
	if (sh->text_size <= shrink_size && sh->dbuf_size > INITIAL_SIZE) {
		/* Shrink data buffer. */
		newp = realloc(sh->data, shrink_size);
		if (newp == NULL) {
			/* Failed to shrink buffer... no matter. */
			return EOK;
		}

		sh->data = newp;
		sh->dbuf_size = shrink_size;
	}

	return EOK;
}

/** Read text from sheet. */
void sheet_copy_out(sheet_t *sh, spt_t const *spos, spt_t const *epos,
    char *buf, size_t bufsize, spt_t *fpos)
{
	char *spp;
	size_t range_sz;
	size_t copy_sz;
	size_t off, prev;
	wchar_t c;

	spp = sh->data + spos->b_off;
	range_sz = epos->b_off - spos->b_off;
	copy_sz = (range_sz < bufsize - 1) ? range_sz : bufsize - 1;

	prev = off = 0;
	do {
		prev = off;
		c = str_decode(spp, &off, copy_sz);
	} while (c != '\0');

	/* Crop copy_sz down to the last full character. */
	copy_sz = prev;

	memcpy(buf, spp, copy_sz);
	buf[copy_sz] = '\0';

	fpos->b_off = spos->b_off + copy_sz;
	fpos->sh = sh;
}

/** Get point preceding or following character cell. */
void sheet_get_cell_pt(sheet_t *sh, coord_t const *coord, enum dir_spec dir,
    spt_t *pt)
{
	size_t cur_pos, prev_pos;
	wchar_t c;
	coord_t cc;

	cc.row = cc.column = 1;
	cur_pos = prev_pos = 0;
	while (true) {
		if (prev_pos >= sh->text_size) {
			/* Cannot advance any further. */
			break;
		}

		if ((cc.row >= coord->row && cc.column > coord->column) ||
		    cc.row > coord->row) {
			/* We are past the requested coordinates. */
			break;
		}

		prev_pos = cur_pos;

		c = str_decode(sh->data, &cur_pos, sh->text_size);
		if (c == '\n') {
			++cc.row;
			cc.column = 1;
		} else if (c == '\t') {
			cc.column = 1 + ALIGN_UP(cc.column, TAB_WIDTH);
		} else {
			++cc.column;
		}
	}

	pt->sh = sh;
	pt->b_off = (dir == dir_before) ? prev_pos : cur_pos;
}

/** Get the number of character cells a row occupies. */
void sheet_get_row_width(sheet_t *sh, int row, int *length)
{
	coord_t coord;
	spt_t pt;

	/* Especially nasty hack */
	coord.row = row;
	coord.column = 65536;

	sheet_get_cell_pt(sh, &coord, dir_before, &pt);
	spt_get_coord(&pt, &coord);
	*length = coord.column;
}

/** Get the number of rows in a sheet. */
void sheet_get_num_rows(sheet_t *sh, int *rows)
{
	int cnt;
	size_t bo;

	cnt = 1;
	for (bo = 0; bo < sh->dbuf_size; ++bo) {
		if (sh->data[bo] == '\n')
			cnt += 1;
	}

	*rows = cnt;
}

/** Get the coordinates of an s-point. */
void spt_get_coord(spt_t const *pos, coord_t *coord)
{
	size_t off;
	coord_t cc;
	wchar_t c;
	sheet_t *sh;

	sh = pos->sh;
	cc.row = cc.column = 1;

	off = 0;
	while (off < pos->b_off && off < sh->text_size) {
		c = str_decode(sh->data, &off, sh->text_size);
		if (c == '\n') {
			++cc.row;
			cc.column = 1;
		} else if (c == '\t') {
			cc.column = 1 + ALIGN_UP(cc.column, TAB_WIDTH);
		} else {
			++cc.column;
		}
	}

	*coord = cc;
}

/** Test if two s-points are equal. */
bool spt_equal(spt_t const *a, spt_t const *b)
{
	return a->b_off == b->b_off;
}

/** Get a character at spt and return next spt */
wchar_t spt_next_char(spt_t spt, spt_t *next)
{
	wchar_t ch = str_decode(spt.sh->data, &spt.b_off, spt.sh->text_size);
	if (next)
		*next = spt;
	return ch;
}

wchar_t spt_prev_char(spt_t spt, spt_t *prev)
{
	wchar_t ch = str_decode_reverse(spt.sh->data, &spt.b_off, spt.sh->text_size);
	if (prev)
		*prev = spt;
	return ch;
}

/** Place a tag on the specified s-point. */
void sheet_place_tag(sheet_t *sh, spt_t const *pt, tag_t *tag)
{
	tag->b_off = pt->b_off;
	tag->sh = sh;
	list_append(&tag->link, &sh->tags);
}

/** Remove a tag from the sheet. */
void sheet_remove_tag(sheet_t *sh, tag_t *tag)
{
	list_remove(&tag->link);
}

/** Get s-point on which the tag is located right now. */
void tag_get_pt(tag_t const *tag, spt_t *pt)
{
	pt->b_off = tag->b_off;
	pt->sh = tag->sh;
}

/** @}
 */
