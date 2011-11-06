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
 * @brief Text editor.
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <vfs/vfs.h>
#include <io/console.h>
#include <io/style.h>
#include <io/keycode.h>
#include <errno.h>
#include <align.h>
#include <macros.h>
#include <clipboard.h>
#include <bool.h>

#include "sheet.h"

enum redraw_flags {
	REDRAW_TEXT	= (1 << 0),
	REDRAW_ROW	= (1 << 1),
	REDRAW_STATUS	= (1 << 2),
	REDRAW_CARET	= (1 << 3)
};

/** Pane
 *
 * A rectangular area of the screen used to edit a document. Different
 * panes can be possibly used to edit the same document.
 */
typedef struct {
	/* Pane dimensions */
	int rows, columns;

	/* Position of the visible area */
	int sh_row, sh_column;

	/** Bitmask of components that need redrawing */
	enum redraw_flags rflags;

	/** Current position of the caret */
	tag_t caret_pos;

	/** Start of selection */
	tag_t sel_start;

	/** 
	 * Ideal column where the caret should try to get. This is used
	 * for maintaining the same column during vertical movement.
	 */
	int ideal_column;
} pane_t;

/** Document
 *
 * Associates a sheet with a file where it can be saved to.
 */
typedef struct {
	char *file_name;
	sheet_t sh;
} doc_t;

static console_ctrl_t *con;
static doc_t doc;
static bool done;
static pane_t pane;
static bool cursor_visible;

static sysarg_t scr_rows;
static sysarg_t scr_columns;

#define ROW_BUF_SIZE 4096
#define BUF_SIZE 64
#define TAB_WIDTH 8
#define ED_INFTY 65536

/** Maximum filename length that can be entered. */
#define INFNAME_MAX_LEN 128

static void cursor_show(void);
static void cursor_hide(void);
static void cursor_setvis(bool visible);

static void key_handle_unmod(kbd_event_t const *ev);
static void key_handle_ctrl(kbd_event_t const *ev);
static void key_handle_shift(kbd_event_t const *ev);
static void key_handle_movement(unsigned int key, bool shift);

static int file_save(char const *fname);
static void file_save_as(void);
static int file_insert(char *fname);
static int file_save_range(char const *fname, spt_t const *spos,
    spt_t const *epos);
static char *filename_prompt(char const *prompt, char const *init_value);
static char *range_get_str(spt_t const *spos, spt_t const *epos);

static void pane_text_display(void);
static void pane_row_display(void);
static void pane_row_range_display(int r0, int r1);
static void pane_status_display(void);
static void pane_caret_display(void);

static void insert_char(wchar_t c);
static void delete_char_before(void);
static void delete_char_after(void);
static void caret_update(void);
static void caret_move(int drow, int dcolumn, enum dir_spec align_dir);

static bool selection_active(void);
static void selection_sel_all(void);
static void selection_get_points(spt_t *pa, spt_t *pb);
static void selection_delete(void);
static void selection_copy(void);
static void insert_clipboard_data(void);

static void pt_get_sof(spt_t *pt);
static void pt_get_eof(spt_t *pt);
static int tag_cmp(tag_t const *a, tag_t const *b);
static int spt_cmp(spt_t const *a, spt_t const *b);
static int coord_cmp(coord_t const *a, coord_t const *b);

static void status_display(char const *str);


int main(int argc, char *argv[])
{
	kbd_event_t ev;
	coord_t coord;
	bool new_file;

	spt_t pt;

	con = console_init(stdin, stdout);
	console_clear(con);

	console_get_size(con, &scr_columns, &scr_rows);

	pane.rows = scr_rows - 1;
	pane.columns = scr_columns;
	pane.sh_row = 1;
	pane.sh_column = 1;

	/* Start with an empty sheet. */
	sheet_init(&doc.sh);

	/* Place caret at the beginning of file. */
	coord.row = coord.column = 1;
	sheet_get_cell_pt(&doc.sh, &coord, dir_before, &pt);
	sheet_place_tag(&doc.sh, &pt, &pane.caret_pos);
	pane.ideal_column = coord.column;

	if (argc == 2) {
		doc.file_name = str_dup(argv[1]);
	} else if (argc > 1) {
		printf("Invalid arguments.\n");
		return -2;
	} else {
		doc.file_name = NULL;
	}

	new_file = false;

	if (doc.file_name == NULL || file_insert(doc.file_name) != EOK)
		new_file = true;

	/* Move to beginning of file. */
	caret_move(-ED_INFTY, -ED_INFTY, dir_before);

	/* Place selection start tag. */
	tag_get_pt(&pane.caret_pos, &pt);
	sheet_place_tag(&doc.sh, &pt, &pane.sel_start);

	/* Initial display */
	cursor_visible = true;

	cursor_hide();
	console_clear(con);
	pane_text_display();
	pane_status_display();
	if (new_file && doc.file_name != NULL)
		status_display("File not found. Starting empty file.");
	pane_caret_display();
	cursor_show();

	done = false;

	while (!done) {
		console_get_kbd_event(con, &ev);
		pane.rflags = 0;

		if (ev.type == KEY_PRESS) {
			/* Handle key press. */
			if (((ev.mods & KM_ALT) == 0) &&
			    ((ev.mods & KM_SHIFT) == 0) &&
			     (ev.mods & KM_CTRL) != 0) {
				key_handle_ctrl(&ev);
			} else if (((ev.mods & KM_ALT) == 0) &&
			    ((ev.mods & KM_CTRL) == 0) &&
			     (ev.mods & KM_SHIFT) != 0) {
				key_handle_shift(&ev);
			} else if ((ev.mods & (KM_CTRL | KM_ALT | KM_SHIFT)) == 0) {
				key_handle_unmod(&ev);
			}
		}

		/* Redraw as necessary. */

		cursor_hide();

		if (pane.rflags & REDRAW_TEXT)
			pane_text_display();
		if (pane.rflags & REDRAW_ROW)
			pane_row_display();
		if (pane.rflags & REDRAW_STATUS)
			pane_status_display();
		if (pane.rflags & REDRAW_CARET)
			pane_caret_display();

		cursor_show();
	}

	console_clear(con);

	return 0;
}

static void cursor_show(void)
{
	cursor_setvis(true);
}

static void cursor_hide(void)
{
	cursor_setvis(false);
}

static void cursor_setvis(bool visible)
{
	if (cursor_visible != visible) {
		console_cursor_visibility(con, visible);
		cursor_visible = visible;
	}
}

/** Handle key without modifier. */
static void key_handle_unmod(kbd_event_t const *ev)
{
	switch (ev->key) {
	case KC_ENTER:
		selection_delete();
		insert_char('\n');
		caret_update();
		break;
	case KC_LEFT:
	case KC_RIGHT:
	case KC_UP:
	case KC_DOWN:
	case KC_HOME:
	case KC_END:
	case KC_PAGE_UP:
	case KC_PAGE_DOWN:
		key_handle_movement(ev->key, false);
		break;
	case KC_BACKSPACE:
		if (selection_active())
			selection_delete();
		else
			delete_char_before();
		caret_update();
		break;
	case KC_DELETE:
		if (selection_active())
			selection_delete();
		else
			delete_char_after();
		caret_update();
		break;
	default:
		if (ev->c >= 32 || ev->c == '\t') {
			selection_delete();
			insert_char(ev->c);
			caret_update();
		}
		break;
	}
}

/** Handle Shift-key combination. */
static void key_handle_shift(kbd_event_t const *ev)
{
	switch (ev->key) {
	case KC_LEFT:
	case KC_RIGHT:
	case KC_UP:
	case KC_DOWN:
	case KC_HOME:
	case KC_END:
	case KC_PAGE_UP:
	case KC_PAGE_DOWN:
		key_handle_movement(ev->key, true);
		break;
	default:
		if (ev->c >= 32 || ev->c == '\t') {
			selection_delete();
			insert_char(ev->c);
			caret_update();
		}
		break;
	}
}

/** Handle Ctrl-key combination. */
static void key_handle_ctrl(kbd_event_t const *ev)
{
	switch (ev->key) {
	case KC_Q:
		done = true;
		break;
	case KC_S:
		if (doc.file_name != NULL)
			file_save(doc.file_name);
		else
			file_save_as();
		break;
	case KC_E:
		file_save_as();
		break;
	case KC_C:
		selection_copy();
		break;
	case KC_V:
		selection_delete();
		insert_clipboard_data();
		pane.rflags |= REDRAW_TEXT;
		caret_update();
		break;
	case KC_X:
		selection_copy();
		selection_delete();
		pane.rflags |= REDRAW_TEXT;
		caret_update();
		break;
	case KC_A:
		selection_sel_all();
		break;
	default:
		break;
	}
}

static void key_handle_movement(unsigned int key, bool select)
{
	spt_t pt;
	spt_t caret_pt;
	coord_t c_old, c_new;
	bool had_sel;

	/* Check if we had selection before. */
	tag_get_pt(&pane.caret_pos, &caret_pt);
	tag_get_pt(&pane.sel_start, &pt);
	had_sel = !spt_equal(&caret_pt, &pt);

	switch (key) {
	case KC_LEFT:
		caret_move(0, -1, dir_before);
		break;
	case KC_RIGHT:
		caret_move(0, 0, dir_after);
		break;
	case KC_UP:
		caret_move(-1, 0, dir_before);
		break;
	case KC_DOWN:
		caret_move(+1, 0, dir_before);
		break;
	case KC_HOME:
		caret_move(0, -ED_INFTY, dir_before);
		break;
	case KC_END:
		caret_move(0, +ED_INFTY, dir_before);
		break;
	case KC_PAGE_UP:
		caret_move(-pane.rows, 0, dir_before);
		break;
	case KC_PAGE_DOWN:
		caret_move(+pane.rows, 0, dir_before);
		break;
	default:
		break;
	}

	if (select == false) {
		/* Move sel_start to the same point as caret. */
		sheet_remove_tag(&doc.sh, &pane.sel_start);
		tag_get_pt(&pane.caret_pos, &pt);
		sheet_place_tag(&doc.sh, &pt, &pane.sel_start);
	}

	if (select) {
		tag_get_pt(&pane.caret_pos, &pt);
		spt_get_coord(&caret_pt, &c_old);
		spt_get_coord(&pt, &c_new);

		if (c_old.row == c_new.row)
			pane.rflags |= REDRAW_ROW;
		else
			pane.rflags |= REDRAW_TEXT;

	} else if (had_sel == true) {
		/* Redraw because text was unselected. */
		pane.rflags |= REDRAW_TEXT;
	}
}

/** Save the document. */
static int file_save(char const *fname)
{
	spt_t sp, ep;
	int rc;

	status_display("Saving...");
	pt_get_sof(&sp);
	pt_get_eof(&ep);

	rc = file_save_range(fname, &sp, &ep);

	switch (rc) {
	case EINVAL:
		status_display("Error opening file!");
		break;
	case EIO:
		status_display("Error writing data!");
		break;
	default:
		status_display("File saved.");
		break;
	}

	return rc;
}

/** Change document name and save. */
static void file_save_as(void)
{
	const char *old_fname = (doc.file_name != NULL) ? doc.file_name : "";
	char *fname;
	
	fname = filename_prompt("Save As", old_fname);
	if (fname == NULL) {
		status_display("Save cancelled.");
		return;
	}

	int rc = file_save(fname);
	if (rc != EOK)
		return;

	if (doc.file_name != NULL)
		free(doc.file_name);
	doc.file_name = fname;
}

/** Ask for a file name. */
static char *filename_prompt(char const *prompt, char const *init_value)
{
	kbd_event_t ev;
	char *str;
	wchar_t buffer[INFNAME_MAX_LEN + 1];
	int max_len;
	int nc;
	bool done;

	asprintf(&str, "%s: %s", prompt, init_value);
	status_display(str);
	console_set_pos(con, 1 + str_length(str), scr_rows - 1);
	free(str);

	console_set_style(con, STYLE_INVERTED);

	max_len = min(INFNAME_MAX_LEN, scr_columns - 4 - str_length(prompt));
	str_to_wstr(buffer, max_len + 1, init_value);
	nc = wstr_length(buffer);
	done = false;

	while (!done) {
		console_get_kbd_event(con, &ev);

		if (ev.type == KEY_PRESS) {
			/* Handle key press. */
			if (((ev.mods & KM_ALT) == 0) &&
			     (ev.mods & KM_CTRL) != 0) {
				;
			} else if ((ev.mods & (KM_CTRL | KM_ALT)) == 0) {
				switch (ev.key) {
				case KC_ESCAPE:
					return NULL;
				case KC_BACKSPACE:
					if (nc > 0) {
						putchar('\b');
						console_flush(con);
						--nc;
					}
					break;
				case KC_ENTER:
					done = true;
					break;
				default:
					if (ev.c >= 32 && nc < max_len) {
						putchar(ev.c);
						console_flush(con);
						buffer[nc++] = ev.c;
					}
					break;
				}
			}
		}
	}

	buffer[nc] = '\0';
	str = wstr_to_astr(buffer);

	console_set_style(con, STYLE_NORMAL);

	return str;
}

/** Insert file at caret position.
 *
 * Reads in the contents of a file and inserts them at the current position
 * of the caret.
 */
static int file_insert(char *fname)
{
	FILE *f;
	wchar_t c;
	char buf[BUF_SIZE];
	int bcnt;
	int n_read;
	size_t off;

	f = fopen(fname, "rt");
	if (f == NULL)
		return EINVAL;

	bcnt = 0;

	while (true) {
		if (bcnt < STR_BOUNDS(1)) {
			n_read = fread(buf + bcnt, 1, BUF_SIZE - bcnt, f);
			bcnt += n_read;
		}

		off = 0;
		c = str_decode(buf, &off, bcnt);
		if (c == '\0')
			break;

		bcnt -= off;
		memcpy(buf, buf + off, bcnt);

		insert_char(c);
	}

	fclose(f);

	return EOK;
}

/** Save a range of text into a file. */
static int file_save_range(char const *fname, spt_t const *spos,
    spt_t const *epos)
{
	FILE *f;
	char buf[BUF_SIZE];
	spt_t sp, bep;
	size_t bytes, n_written;

	f = fopen(fname, "wt");
	if (f == NULL)
		return EINVAL;

	sp = *spos;

	do {
		sheet_copy_out(&doc.sh, &sp, epos, buf, BUF_SIZE, &bep);
		bytes = str_size(buf);

		n_written = fwrite(buf, 1, bytes, f);
		if (n_written != bytes) {
			return EIO;
		}

		sp = bep;
	} while (!spt_equal(&bep, epos));

	if (fclose(f) != EOK)
		return EIO;

	return EOK;
}

/** Return contents of range as a new string. */
static char *range_get_str(spt_t const *spos, spt_t const *epos)
{
	char *buf;
	spt_t sp, bep;
	size_t bytes;
	size_t buf_size, bpos;

	buf_size = 1;

	buf = malloc(buf_size);
	if (buf == NULL)
		return NULL;

	bpos = 0;
	sp = *spos;

	while (true) {
		sheet_copy_out(&doc.sh, &sp, epos, &buf[bpos], buf_size - bpos,
		    &bep);
		bytes = str_size(&buf[bpos]);
		bpos += bytes;
		sp = bep;

		if (spt_equal(&bep, epos))
			break;

		buf_size *= 2;
		buf = realloc(buf, buf_size);
		if (buf == NULL)
			return NULL;
	}

	return buf;
}

static void pane_text_display(void)
{
	int sh_rows, rows;

	sheet_get_num_rows(&doc.sh, &sh_rows);
	rows = min(sh_rows - pane.sh_row + 1, pane.rows);

	/* Draw rows from the sheet. */

	console_set_pos(con, 0, 0);
	pane_row_range_display(0, rows);

	/* Clear the remaining rows if file is short. */
	
	int i;
	sysarg_t j;
	for (i = rows; i < pane.rows; ++i) {
		console_set_pos(con, 0, i);
		for (j = 0; j < scr_columns; ++j)
			putchar(' ');
		console_flush(con);
	}

	pane.rflags |= (REDRAW_STATUS | REDRAW_CARET);
	pane.rflags &= ~REDRAW_ROW;
}

/** Display just the row where the caret is. */
static void pane_row_display(void)
{
	spt_t caret_pt;
	coord_t coord;
	int ridx;

	tag_get_pt(&pane.caret_pos, &caret_pt);
	spt_get_coord(&caret_pt, &coord);

	ridx = coord.row - pane.sh_row;
	pane_row_range_display(ridx, ridx + 1);
	pane.rflags |= (REDRAW_STATUS | REDRAW_CARET);
}

static void pane_row_range_display(int r0, int r1)
{
	int i, j, fill;
	spt_t rb, re, dep, pt;
	coord_t rbc, rec;
	char row_buf[ROW_BUF_SIZE];
	wchar_t c;
	size_t pos, size;
	int s_column;
	coord_t csel_start, csel_end, ctmp;

	/* Determine selection start and end. */

	tag_get_pt(&pane.sel_start, &pt);
	spt_get_coord(&pt, &csel_start);

	tag_get_pt(&pane.caret_pos, &pt);
	spt_get_coord(&pt, &csel_end);

	if (coord_cmp(&csel_start, &csel_end) > 0) {
		ctmp = csel_start;
		csel_start = csel_end;
		csel_end = ctmp;
	}

	/* Draw rows from the sheet. */

	console_set_pos(con, 0, 0);
	for (i = r0; i < r1; ++i) {
		/* Starting point for row display */
		rbc.row = pane.sh_row + i;
		rbc.column = pane.sh_column;
		sheet_get_cell_pt(&doc.sh, &rbc, dir_before, &rb);

		/* Ending point for row display */
		rec.row = pane.sh_row + i;
		rec.column = pane.sh_column + pane.columns;
		sheet_get_cell_pt(&doc.sh, &rec, dir_before, &re);

		/* Copy the text of the row to the buffer. */
		sheet_copy_out(&doc.sh, &rb, &re, row_buf, ROW_BUF_SIZE, &dep);

		/* Display text from the buffer. */

		if (coord_cmp(&csel_start, &rbc) <= 0 &&
		    coord_cmp(&rbc, &csel_end) < 0) {
			console_flush(con);
			console_set_style(con, STYLE_SELECTED);
			console_flush(con);
		}

		console_set_pos(con, 0, i);
		size = str_size(row_buf);
		pos = 0;
		s_column = pane.sh_column;
		while (pos < size) {
			if ((csel_start.row == rbc.row) && (csel_start.column == s_column)) {
				console_flush(con);
				console_set_style(con, STYLE_SELECTED);
				console_flush(con);
			}
	
			if ((csel_end.row == rbc.row) && (csel_end.column == s_column)) {
				console_flush(con);
				console_set_style(con, STYLE_NORMAL);
				console_flush(con);
			}
	
			c = str_decode(row_buf, &pos, size);
			if (c != '\t') {
				printf("%lc", (wint_t) c);
				s_column += 1;
			} else {
				fill = 1 + ALIGN_UP(s_column, TAB_WIDTH)
				    - s_column;

				for (j = 0; j < fill; ++j)
					putchar(' ');
				s_column += fill;
			}
		}

		if ((csel_end.row == rbc.row) && (csel_end.column == s_column)) {
			console_flush(con);
			console_set_style(con, STYLE_NORMAL);
			console_flush(con);
		}

		/* Fill until the end of display area. */

		if (str_length(row_buf) < (unsigned) scr_columns)
			fill = scr_columns - str_length(row_buf);
		else
			fill = 0;

		for (j = 0; j < fill; ++j)
			putchar(' ');
		console_flush(con);
		console_set_style(con, STYLE_NORMAL);
	}

	pane.rflags |= REDRAW_CARET;
}

/** Display pane status in the status line. */
static void pane_status_display(void)
{
	spt_t caret_pt;
	coord_t coord;

	tag_get_pt(&pane.caret_pos, &caret_pt);
	spt_get_coord(&caret_pt, &coord);

	const char *fname = (doc.file_name != NULL) ? doc.file_name : "<unnamed>";

	console_set_pos(con, 0, scr_rows - 1);
	console_set_style(con, STYLE_INVERTED);
	int n = printf(" %d, %d: File '%s'. Ctrl-Q Quit  Ctrl-S Save  "
	    "Ctrl-E Save As", coord.row, coord.column, fname);
	
	int pos = scr_columns - 1 - n;
	printf("%*s", pos, "");
	console_flush(con);
	console_set_style(con, STYLE_NORMAL);

	pane.rflags |= REDRAW_CARET;
}

/** Set cursor to reflect position of the caret. */
static void pane_caret_display(void)
{
	spt_t caret_pt;
	coord_t coord;

	tag_get_pt(&pane.caret_pos, &caret_pt);

	spt_get_coord(&caret_pt, &coord);
	console_set_pos(con, coord.column - pane.sh_column,
	    coord.row - pane.sh_row);
}

/** Insert a character at caret position. */
static void insert_char(wchar_t c)
{
	spt_t pt;
	char cbuf[STR_BOUNDS(1) + 1];
	size_t offs;

	tag_get_pt(&pane.caret_pos, &pt);

	offs = 0;
	chr_encode(c, cbuf, &offs, STR_BOUNDS(1) + 1);
	cbuf[offs] = '\0';

	(void) sheet_insert(&doc.sh, &pt, dir_before, cbuf);

	pane.rflags |= REDRAW_ROW;
	if (c == '\n')
		pane.rflags |= REDRAW_TEXT;
}

/** Delete the character before the caret. */
static void delete_char_before(void)
{
	spt_t sp, ep;
	coord_t coord;

	tag_get_pt(&pane.caret_pos, &ep);
	spt_get_coord(&ep, &coord);

	coord.column -= 1;
	sheet_get_cell_pt(&doc.sh, &coord, dir_before, &sp);

	(void) sheet_delete(&doc.sh, &sp, &ep);

	pane.rflags |= REDRAW_ROW;
	if (coord.column < 1)
		pane.rflags |= REDRAW_TEXT;
}

/** Delete the character after the caret. */
static void delete_char_after(void)
{
	spt_t sp, ep;
	coord_t sc, ec;

	tag_get_pt(&pane.caret_pos, &sp);
	spt_get_coord(&sp, &sc);

	sheet_get_cell_pt(&doc.sh, &sc, dir_after, &ep);
	spt_get_coord(&ep, &ec);

	(void) sheet_delete(&doc.sh, &sp, &ep);

	pane.rflags |= REDRAW_ROW;
	if (ec.row != sc.row)
		pane.rflags |= REDRAW_TEXT;
}

/** Scroll pane after caret has moved.
 *
 * After modifying the position of the caret, this is called to scroll
 * the pane to ensure that the caret is in the visible area.
 */
static void caret_update(void)
{
	spt_t pt;
	coord_t coord;

	tag_get_pt(&pane.caret_pos, &pt);
	spt_get_coord(&pt, &coord);

	/* Scroll pane vertically. */

	if (coord.row < pane.sh_row) {
		pane.sh_row = coord.row;
		pane.rflags |= REDRAW_TEXT;
	}

	if (coord.row > pane.sh_row + pane.rows - 1) {
		pane.sh_row = coord.row - pane.rows + 1;
		pane.rflags |= REDRAW_TEXT;
	}

	/* Scroll pane horizontally. */

	if (coord.column < pane.sh_column) {
		pane.sh_column = coord.column;
		pane.rflags |= REDRAW_TEXT;
	}

	if (coord.column > pane.sh_column + pane.columns - 1) {
		pane.sh_column = coord.column - pane.columns + 1;
		pane.rflags |= REDRAW_TEXT;
	}

	pane.rflags |= (REDRAW_CARET | REDRAW_STATUS);
}

/** Change the caret position.
 *
 * Moves caret relatively to the current position. Looking at the first
 * character cell after the caret and moving by @a drow and @a dcolumn, we get
 * to a new character cell, and thus a new character. Then we either go to the
 * point before the the character or after it, depending on @a align_dir.
 */
static void caret_move(int drow, int dcolumn, enum dir_spec align_dir)
{
	spt_t pt;
	coord_t coord;
	int num_rows;
	bool pure_vertical;

	tag_get_pt(&pane.caret_pos, &pt);
	spt_get_coord(&pt, &coord);
	coord.row += drow; coord.column += dcolumn;

	/* Clamp coordinates. */
	if (drow < 0 && coord.row < 1) coord.row = 1;
	if (dcolumn < 0 && coord.column < 1) coord.column = 1;
	if (drow > 0) {
		sheet_get_num_rows(&doc.sh, &num_rows);
		if (coord.row > num_rows) coord.row = num_rows;
	}

	/* For purely vertical movement try attaining @c ideal_column. */
	pure_vertical = (dcolumn == 0 && align_dir == dir_before);
	if (pure_vertical)
		coord.column = pane.ideal_column;

	/*
	 * Select the point before or after the character at the designated
	 * coordinates. The character can be wider than one cell (e.g. tab).
	 */
	sheet_get_cell_pt(&doc.sh, &coord, align_dir, &pt);
	sheet_remove_tag(&doc.sh, &pane.caret_pos);
	sheet_place_tag(&doc.sh, &pt, &pane.caret_pos);

	/* For non-vertical movement set the new value for @c ideal_column. */
	if (!pure_vertical) {
		spt_get_coord(&pt, &coord);
		pane.ideal_column = coord.column;
	}

	caret_update();
}

/** Check for non-empty selection. */
static bool selection_active(void)
{
	return (tag_cmp(&pane.caret_pos, &pane.sel_start) != 0);
}

static void selection_get_points(spt_t *pa, spt_t *pb)
{
	spt_t pt;

	tag_get_pt(&pane.sel_start, pa);
	tag_get_pt(&pane.caret_pos, pb);

	if (spt_cmp(pa, pb) > 0) {
		pt = *pa;
		*pa = *pb;
		*pb = pt;
	}
}

/** Delete selected text. */
static void selection_delete(void)
{
	spt_t pa, pb;
	coord_t ca, cb;
	int rel;

	tag_get_pt(&pane.sel_start, &pa);
	tag_get_pt(&pane.caret_pos, &pb);
	spt_get_coord(&pa, &ca);
	spt_get_coord(&pb, &cb);
	rel = coord_cmp(&ca, &cb);

	if (rel == 0)
		return;

	if (rel < 0)
		sheet_delete(&doc.sh, &pa, &pb);
	else
		sheet_delete(&doc.sh, &pb, &pa);

	if (ca.row == cb.row)
		pane.rflags |= REDRAW_ROW;
	else
		pane.rflags |= REDRAW_TEXT;
}

static void selection_sel_all(void)
{
	spt_t spt, ept;

	pt_get_sof(&spt);
	pt_get_eof(&ept);
	sheet_remove_tag(&doc.sh, &pane.sel_start);
	sheet_place_tag(&doc.sh, &spt, &pane.sel_start);
	sheet_remove_tag(&doc.sh, &pane.caret_pos);
	sheet_place_tag(&doc.sh, &ept, &pane.caret_pos);

	pane.rflags |= REDRAW_TEXT;
	caret_update();
}

static void selection_copy(void)
{
	spt_t pa, pb;
	char *str;

	selection_get_points(&pa, &pb);
	str = range_get_str(&pa, &pb);
	if (str == NULL || clipboard_put_str(str) != EOK) {
		status_display("Copying to clipboard failed!");
	}
	free(str);
}

static void insert_clipboard_data(void)
{
	char *str;
	size_t off;
	wchar_t c;
	int rc;

	rc = clipboard_get_str(&str);
	if (rc != EOK || str == NULL)
		return;

	off = 0;

	while (true) {
		c = str_decode(str, &off, STR_NO_LIMIT);
		if (c == '\0')
			break;

		insert_char(c);
	}

	free(str);
}

/** Get start-of-file s-point. */
static void pt_get_sof(spt_t *pt)
{
	coord_t coord;

	coord.row = coord.column = 1;
	sheet_get_cell_pt(&doc.sh, &coord, dir_before, pt);
}

/** Get end-of-file s-point. */
static void pt_get_eof(spt_t *pt)
{
	coord_t coord;
	int num_rows;

	sheet_get_num_rows(&doc.sh, &num_rows);
	coord.row = num_rows + 1;
	coord.column = 1;

	sheet_get_cell_pt(&doc.sh, &coord, dir_after, pt);
}

/** Compare tags. */
static int tag_cmp(tag_t const *a, tag_t const *b)
{
	spt_t pa, pb;

	tag_get_pt(a, &pa);
	tag_get_pt(b, &pb);

	return spt_cmp(&pa, &pb);
}

/** Compare s-points. */
static int spt_cmp(spt_t const *a, spt_t const *b)
{
	coord_t ca, cb;

	spt_get_coord(a, &ca);
	spt_get_coord(b, &cb);

	return coord_cmp(&ca, &cb);
}

/** Compare coordinats. */
static int coord_cmp(coord_t const *a, coord_t const *b)
{
	if (a->row - b->row != 0)
		return a->row - b->row;

	return a->column - b->column;
}

/** Display text in the status line. */
static void status_display(char const *str)
{
	console_set_pos(con, 0, scr_rows - 1);
	console_set_style(con, STYLE_INVERTED);
	
	int pos = -(scr_columns - 3);
	printf(" %*s ", pos, str);
	console_flush(con);
	console_set_style(con, STYLE_NORMAL);

	pane.rflags |= REDRAW_CARET;
}

/** @}
 */
