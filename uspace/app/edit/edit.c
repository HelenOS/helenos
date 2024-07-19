/*
 * Copyright (c) 2024 Jiri Svoboda
 * Copyright (c) 2012 Martin Sucha
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

#include <align.h>
#include <clipboard.h>
#include <errno.h>
#include <gfx/color.h>
#include <gfx/cursor.h>
#include <gfx/font.h>
#include <gfx/render.h>
#include <gfx/text.h>
#include <io/kbd_event.h>
#include <io/keycode.h>
#include <io/pos_event.h>
#include <io/style.h>
#include <macros.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>
#include <types/common.h>
#include <ui/control.h>
#include <ui/filedialog.h>
#include <ui/fixed.h>
#include <ui/label.h>
#include <ui/menu.h>
#include <ui/menubar.h>
#include <ui/menudd.h>
#include <ui/menuentry.h>
#include <ui/promptdialog.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include <vfs/vfs.h>

#include "sheet.h"
#include "search.h"

enum redraw_flags {
	REDRAW_TEXT	= (1 << 0),
	REDRAW_ROW	= (1 << 1),
	REDRAW_STATUS	= (1 << 2),
	REDRAW_CARET	= (1 << 3)
};

/** Pane
 *
 * A rectangular area of the screen used to edit a document. Different
 * panes can be possibly used to edit the same document. This is a custom
 * UI control.
 */
typedef struct {
	/** Base control object */
	struct ui_control *control;

	/** Containing window */
	ui_window_t *window;

	/** UI resource */
	struct ui_resource *res;

	/** Pane rectangle */
	gfx_rect_t rect;

	/** Pane color */
	gfx_color_t *color;

	/** Selection color */
	gfx_color_t *sel_color;

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

	/** Active keyboard modifiers */
	keymod_t keymod;

	/**
	 * Ideal column where the caret should try to get. This is used
	 * for maintaining the same column during vertical movement.
	 */
	int ideal_column;

	bool search_reverse;
	char *previous_search;
	bool previous_search_reverse;
} pane_t;

/** Text editor */
typedef struct {
	/** User interface */
	ui_t *ui;
	/** Editor window */
	ui_window_t *window;
	/** UI resource */
	ui_resource_t *ui_res;
	/** Menu bar */
	ui_menu_bar_t *menubar;
	/** Status bar */
	ui_label_t *status;
} edit_t;

/** Document
 *
 * Associates a sheet with a file where it can be saved to.
 */
typedef struct {
	char *file_name;
	sheet_t *sh;
} doc_t;

static edit_t edit;
static doc_t doc;
static pane_t pane;

#define ROW_BUF_SIZE 4096
#define BUF_SIZE 64
#define TAB_WIDTH 8

/** Maximum filename length that can be entered. */
#define INFNAME_MAX_LEN 128

static void cursor_setvis(bool visible);

static void key_handle_press(kbd_event_t *ev);
static void key_handle_unmod(kbd_event_t const *ev);
static void key_handle_ctrl(kbd_event_t const *ev);
static void key_handle_shift(kbd_event_t const *ev);
static void key_handle_shift_ctrl(kbd_event_t const *ev);
static void key_handle_movement(unsigned int key, bool shift);

static void pos_handle(pos_event_t *ev);

static errno_t file_new(void);
static void file_open(void);
static errno_t file_open_file(const char *fname);
static errno_t file_save(char const *fname);
static void file_save_as(void);
static errno_t file_insert(const char *fname);
static errno_t file_save_range(char const *fname, spt_t const *spos,
    spt_t const *epos);
static char *range_get_str(spt_t const *spos, spt_t const *epos);

static errno_t pane_init(ui_window_t *, pane_t *);
static void pane_fini(pane_t *);
static ui_control_t *pane_ctl(pane_t *);
static errno_t pane_update(pane_t *);
static errno_t pane_text_display(pane_t *);
static void pane_row_display(void);
static errno_t pane_row_range_display(pane_t *, int r0, int r1);
static void pane_status_display(pane_t *);
static void pane_caret_display(pane_t *);

static void insert_char(char32_t c);
static void delete_char_before(void);
static void delete_char_after(void);
static void caret_update(void);
static void caret_move_relative(int drow, int dcolumn, enum dir_spec align_dir, bool select);
static void caret_move_absolute(int row, int column, enum dir_spec align_dir, bool select);
static void caret_move(spt_t spt, bool select, bool update_ideal_column);
static void caret_move_word_left(bool select);
static void caret_move_word_right(bool select);
static void caret_go_to_line_ask(void);

static bool selection_active(void);
static void selection_sel_all(void);
static void selection_sel_range(spt_t pa, spt_t pb);
static void selection_get_points(spt_t *pa, spt_t *pb);
static void selection_delete(void);
static void selection_copy(void);
static void edit_cut(void);
static void edit_paste(void);
static void insert_clipboard_data(void);

static void search(char *pattern, bool reverse);
static void search_prompt(bool reverse);
static void search_repeat(void);

static void pt_get_sof(spt_t *pt);
static void pt_get_eof(spt_t *pt);
static void pt_get_sol(spt_t *cpt, spt_t *spt);
static void pt_get_eol(spt_t *cpt, spt_t *ept);
static bool pt_is_word_beginning(spt_t *pt);
static bool pt_is_delimiter(spt_t *pt);
static bool pt_is_punctuation(spt_t *pt);
static spt_t pt_find_word_left(spt_t spt);
static spt_t pt_find_word_left(spt_t spt);

static int tag_cmp(tag_t const *a, tag_t const *b);
static int spt_cmp(spt_t const *a, spt_t const *b);
static int coord_cmp(coord_t const *a, coord_t const *b);

static void status_display(char const *str);
static errno_t edit_ui_create(edit_t *);
static void edit_ui_destroy(edit_t *);

static void edit_wnd_close(ui_window_t *, void *);
static void edit_wnd_focus(ui_window_t *, void *, unsigned);
static void edit_wnd_kbd_event(ui_window_t *, void *, kbd_event_t *);
static void edit_wnd_unfocus(ui_window_t *, void *, unsigned);

static ui_window_cb_t edit_window_cb = {
	.close = edit_wnd_close,
	.focus = edit_wnd_focus,
	.kbd = edit_wnd_kbd_event,
	.unfocus = edit_wnd_unfocus
};

static void edit_menubar_activate(ui_menu_bar_t *, void *);
static void edit_menubar_deactivate(ui_menu_bar_t *, void *);

static ui_menu_bar_cb_t edit_menubar_cb = {
	.activate = edit_menubar_activate,
	.deactivate = edit_menubar_deactivate
};

static void edit_file_new(ui_menu_entry_t *, void *);
static void edit_file_open(ui_menu_entry_t *, void *);
static void edit_file_save(ui_menu_entry_t *, void *);
static void edit_file_save_as(ui_menu_entry_t *, void *);
static void edit_file_exit(ui_menu_entry_t *, void *);
static void edit_edit_cut(ui_menu_entry_t *, void *);
static void edit_edit_copy(ui_menu_entry_t *, void *);
static void edit_edit_paste(ui_menu_entry_t *, void *);
static void edit_edit_delete(ui_menu_entry_t *, void *);
static void edit_edit_select_all(ui_menu_entry_t *, void *);
static void edit_search_find(ui_menu_entry_t *, void *);
static void edit_search_reverse_find(ui_menu_entry_t *, void *);
static void edit_search_find_next(ui_menu_entry_t *, void *);
static void edit_search_go_to_line(ui_menu_entry_t *, void *);

static void pane_ctl_destroy(void *);
static errno_t pane_ctl_paint(void *);
static ui_evclaim_t pane_ctl_pos_event(void *, pos_event_t *);

/** Pabe control ops */
ui_control_ops_t pane_ctl_ops = {
	.destroy = pane_ctl_destroy,
	.paint = pane_ctl_paint,
	.pos_event = pane_ctl_pos_event
};

static void open_dialog_bok(ui_file_dialog_t *, void *, const char *);
static void open_dialog_bcancel(ui_file_dialog_t *, void *);
static void open_dialog_close(ui_file_dialog_t *, void *);

static ui_file_dialog_cb_t open_dialog_cb = {
	.bok = open_dialog_bok,
	.bcancel = open_dialog_bcancel,
	.close = open_dialog_close
};

static void save_as_dialog_bok(ui_file_dialog_t *, void *, const char *);
static void save_as_dialog_bcancel(ui_file_dialog_t *, void *);
static void save_as_dialog_close(ui_file_dialog_t *, void *);

static ui_file_dialog_cb_t save_as_dialog_cb = {
	.bok = save_as_dialog_bok,
	.bcancel = save_as_dialog_bcancel,
	.close = save_as_dialog_close
};

static void go_to_line_dialog_bok(ui_prompt_dialog_t *, void *, const char *);
static void go_to_line_dialog_bcancel(ui_prompt_dialog_t *, void *);
static void go_to_line_dialog_close(ui_prompt_dialog_t *, void *);

static ui_prompt_dialog_cb_t go_to_line_dialog_cb = {
	.bok = go_to_line_dialog_bok,
	.bcancel = go_to_line_dialog_bcancel,
	.close =  go_to_line_dialog_close
};

static void search_dialog_bok(ui_prompt_dialog_t *, void *, const char *);
static void search_dialog_bcancel(ui_prompt_dialog_t *, void *);
static void search_dialog_close(ui_prompt_dialog_t *, void *);

static ui_prompt_dialog_cb_t search_dialog_cb = {
	.bok = search_dialog_bok,
	.bcancel = search_dialog_bcancel,
	.close =  search_dialog_close
};

int main(int argc, char *argv[])
{
	errno_t rc;

	pane.sh_row = 1;
	pane.sh_column = 1;

	/* Create UI */
	rc = edit_ui_create(&edit);
	if (rc != EOK)
		return 1;

	if (argc == 2) {
		doc.file_name = str_dup(argv[1]);
		rc = file_open_file(argv[1]);
		if (rc != EOK) {
			status_display("File not found. Starting empty file.");
			rc = file_new();
		}
	} else if (argc > 1) {
		printf("Invalid arguments.\n");
		return -2;
	} else {
		rc = file_new();
	}

	/* Initial display */
	rc = ui_window_paint(edit.window);
	if (rc != EOK) {
		printf("Error painting window.\n");
		return rc;
	}

	ui_run(edit.ui);

	edit_ui_destroy(&edit);
	return 0;
}

/** Create text editor UI.
 *
 * @param edit Editor
 * @return EOK on success or an error code
 */
static errno_t edit_ui_create(edit_t *edit)
{
	errno_t rc;
	ui_wnd_params_t params;
	ui_fixed_t *fixed = NULL;
	ui_menu_t *mfile = NULL;
	ui_menu_t *medit = NULL;
	ui_menu_entry_t *mnew = NULL;
	ui_menu_entry_t *mopen = NULL;
	ui_menu_entry_t *msave = NULL;
	ui_menu_entry_t *msaveas = NULL;
	ui_menu_entry_t *mfsep = NULL;
	ui_menu_entry_t *mexit = NULL;
	ui_menu_entry_t *mcut = NULL;
	ui_menu_entry_t *mcopy = NULL;
	ui_menu_entry_t *mpaste = NULL;
	ui_menu_entry_t *mdelete = NULL;
	ui_menu_entry_t *mesep = NULL;
	ui_menu_entry_t *mselall = NULL;
	ui_menu_t *msearch = NULL;
	ui_menu_entry_t *mfind = NULL;
	ui_menu_entry_t *mfindr = NULL;
	ui_menu_entry_t *mfindn = NULL;
	ui_menu_entry_t *mssep = NULL;
	ui_menu_entry_t *mgoto = NULL;
	gfx_rect_t arect;
	gfx_rect_t rect;

	rc = ui_create(UI_CONSOLE_DEFAULT, &edit->ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n",
		    UI_CONSOLE_DEFAULT);
		goto error;
	}

	ui_wnd_params_init(&params);
	params.caption = "Text Editor";
	params.style &= ~ui_wds_decorated;
	params.placement = ui_wnd_place_full_screen;

	rc = ui_window_create(edit->ui, &params, &edit->window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		goto error;
	}

	ui_window_set_cb(edit->window, &edit_window_cb, (void *) edit);

	edit->ui_res = ui_window_get_res(edit->window);

	rc = ui_fixed_create(&fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		return rc;
	}

	rc = ui_menu_bar_create(edit->ui, edit->window, &edit->menubar);
	if (rc != EOK) {
		printf("Error creating menu bar.\n");
		return rc;
	}

	ui_menu_bar_set_cb(edit->menubar, &edit_menubar_cb, (void *) edit);

	rc = ui_menu_dd_create(edit->menubar, "~F~ile", NULL, &mfile);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	rc = ui_menu_entry_create(mfile, "~N~ew", "Ctrl-N", &mnew);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(mnew, edit_file_new, (void *) edit);

	rc = ui_menu_entry_create(mfile, "~O~pen", "Ctrl-O", &mopen);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(mopen, edit_file_open, (void *) edit);

	rc = ui_menu_entry_create(mfile, "~S~ave", "Ctrl-S", &msave);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(msave, edit_file_save, (void *) edit);

	rc = ui_menu_entry_create(mfile, "Save ~A~s", "Ctrl-E", &msaveas);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(msaveas, edit_file_save_as, (void *) edit);

	rc = ui_menu_entry_sep_create(mfile, &mfsep);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	rc = ui_menu_entry_create(mfile, "E~x~it", "Ctrl-Q", &mexit);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(mexit, edit_file_exit, (void *) edit);

	rc = ui_menu_dd_create(edit->menubar, "~E~dit", NULL, &medit);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	rc = ui_menu_entry_create(medit, "Cu~t~", "Ctrl-X", &mcut);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(mcut, edit_edit_cut, (void *) edit);

	rc = ui_menu_entry_create(medit, "~C~opy", "Ctrl-C", &mcopy);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(mcopy, edit_edit_copy, (void *) edit);

	rc = ui_menu_entry_create(medit, "~P~aste", "Ctrl-V", &mpaste);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(mpaste, edit_edit_paste, (void *) edit);

	rc = ui_menu_entry_create(medit, "~D~elete", "Del", &mdelete);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(mdelete, edit_edit_delete, (void *) edit);

	rc = ui_menu_entry_sep_create(medit, &mesep);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	rc = ui_menu_entry_create(medit, "Select ~A~ll", "Ctrl-A", &mselall);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(mselall, edit_edit_select_all, (void *) edit);

	rc = ui_menu_dd_create(edit->menubar, "~S~earch", NULL, &msearch);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	rc = ui_menu_entry_create(msearch, "~F~ind", "Ctrl-F", &mfind);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(mfind, edit_search_find, (void *) edit);

	rc = ui_menu_entry_create(msearch, "~R~everse Find", "Ctrl-Shift-F", &mfindr);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(mfindr, edit_search_reverse_find, (void *) edit);

	rc = ui_menu_entry_create(msearch, "Find ~N~ext", "Ctrl-R", &mfindn);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(mfindn, edit_search_find_next, (void *) edit);

	rc = ui_menu_entry_sep_create(msearch, &mssep);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	rc = ui_menu_entry_create(msearch, "Go To ~L~ine", "Ctrl-L", &mgoto);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(mgoto, edit_search_go_to_line, (void *) edit);

	ui_window_get_app_rect(edit->window, &arect);

	rect.p0 = arect.p0;
	rect.p1.x = arect.p1.x;
	rect.p1.y = arect.p0.y + 1;
	ui_menu_bar_set_rect(edit->menubar, &rect);

	rc = ui_fixed_add(fixed, ui_menu_bar_ctl(edit->menubar));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	rc = pane_init(edit->window, &pane);
	if (rc != EOK) {
		printf("Error initializing pane.\n");
		return rc;
	}

	rc = ui_fixed_add(fixed, pane_ctl(&pane));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	rc = ui_label_create(edit->ui_res, "", &edit->status);
	if (rc != EOK) {
		printf("Error creating menu bar.\n");
		return rc;
	}

	rect.p0.x = arect.p0.x;
	rect.p0.y = arect.p1.y - 1;
	rect.p1 = arect.p1;
	ui_label_set_rect(edit->status, &rect);

	rc = ui_fixed_add(fixed, ui_label_ctl(edit->status));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	ui_window_add(edit->window, ui_fixed_ctl(fixed));
	return EOK;
error:
	if (edit->window != NULL)
		ui_window_destroy(edit->window);
	if (edit->ui != NULL)
		ui_destroy(edit->ui);
	return rc;
}

/** Destroy text editor UI.
 *
 * @param edit Editor
 */
static void edit_ui_destroy(edit_t *edit)
{
	ui_window_destroy(edit->window);
	ui_destroy(edit->ui);
}

/* Handle key press. */
static void key_handle_press(kbd_event_t *ev)
{
	if (((ev->mods & KM_ALT) == 0) &&
	    ((ev->mods & KM_SHIFT) == 0) &&
	    (ev->mods & KM_CTRL) != 0) {
		key_handle_ctrl(ev);
	} else if (((ev->mods & KM_ALT) == 0) &&
	    ((ev->mods & KM_CTRL) == 0) &&
	    (ev->mods & KM_SHIFT) != 0) {
		key_handle_shift(ev);
	} else if (((ev->mods & KM_ALT) == 0) &&
	    ((ev->mods & KM_CTRL) != 0) &&
	    (ev->mods & KM_SHIFT) != 0) {
		key_handle_shift_ctrl(ev);
	} else if ((ev->mods & (KM_CTRL | KM_ALT | KM_SHIFT)) == 0) {
		key_handle_unmod(ev);
	}
}

static void cursor_setvis(bool visible)
{
	gfx_context_t *gc = ui_window_get_gc(edit.window);

	(void) gfx_cursor_set_visible(gc, visible);
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
	spt_t pt;
	switch (ev->key) {
	case KC_Q:
		ui_quit(edit.ui);
		break;
	case KC_N:
		file_new();
		break;
	case KC_O:
		file_open();
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
		edit_paste();
		break;
	case KC_X:
		edit_cut();
		break;
	case KC_A:
		selection_sel_all();
		break;
	case KC_RIGHT:
		caret_move_word_right(false);
		break;
	case KC_LEFT:
		caret_move_word_left(false);
		break;
	case KC_L:
		caret_go_to_line_ask();
		break;
	case KC_F:
		search_prompt(false);
		break;
	case KC_R:
		search_repeat();
		break;
	case KC_HOME:
		pt_get_sof(&pt);
		caret_move(pt, false, true);
		break;
	case KC_END:
		pt_get_eof(&pt);
		caret_move(pt, false, true);
		break;
	default:
		break;
	}
}

static void key_handle_shift_ctrl(kbd_event_t const *ev)
{
	spt_t pt;
	switch (ev->key) {
	case KC_LEFT:
		caret_move_word_left(true);
		break;
	case KC_RIGHT:
		caret_move_word_right(true);
		break;
	case KC_F:
		search_prompt(true);
		break;
	case KC_HOME:
		pt_get_sof(&pt);
		caret_move(pt, true, true);
		break;
	case KC_END:
		pt_get_eof(&pt);
		caret_move(pt, true, true);
		break;
	default:
		break;
	}
}

static void pos_handle(pos_event_t *ev)
{
	coord_t bc;
	spt_t pt;
	bool select;

	if (ev->type == POS_PRESS && ev->vpos < (unsigned)pane.rows) {
		bc.row = pane.sh_row + ev->vpos - pane.rect.p0.y;
		bc.column = pane.sh_column + ev->hpos - pane.rect.p0.x;
		sheet_get_cell_pt(doc.sh, &bc, dir_before, &pt);

		select = (pane.keymod & KM_SHIFT) != 0;

		caret_move(pt, select, true);
		pane_update(&pane);
	}
}

/** Move caret while preserving or resetting selection. */
static void caret_move(spt_t new_caret_pt, bool select, bool update_ideal_column)
{
	spt_t old_caret_pt, old_sel_pt;
	coord_t c_old, c_new;
	bool had_sel;

	/* Check if we had selection before. */
	tag_get_pt(&pane.caret_pos, &old_caret_pt);
	tag_get_pt(&pane.sel_start, &old_sel_pt);
	had_sel = !spt_equal(&old_caret_pt, &old_sel_pt);

	/* Place tag of the caret */
	sheet_remove_tag(doc.sh, &pane.caret_pos);
	sheet_place_tag(doc.sh, &new_caret_pt, &pane.caret_pos);

	if (select == false) {
		/* Move sel_start to the same point as caret. */
		sheet_remove_tag(doc.sh, &pane.sel_start);
		sheet_place_tag(doc.sh, &new_caret_pt, &pane.sel_start);
	}

	spt_get_coord(&new_caret_pt, &c_new);
	if (select) {
		spt_get_coord(&old_caret_pt, &c_old);

		if (c_old.row == c_new.row)
			pane.rflags |= REDRAW_ROW;
		else
			pane.rflags |= REDRAW_TEXT;

	} else if (had_sel == true) {
		/* Redraw because text was unselected. */
		pane.rflags |= REDRAW_TEXT;
	}

	if (update_ideal_column)
		pane.ideal_column = c_new.column;

	caret_update();
}

static void key_handle_movement(unsigned int key, bool select)
{
	spt_t pt;
	switch (key) {
	case KC_LEFT:
		caret_move_relative(0, -1, dir_before, select);
		break;
	case KC_RIGHT:
		caret_move_relative(0, 0, dir_after, select);
		break;
	case KC_UP:
		caret_move_relative(-1, 0, dir_before, select);
		break;
	case KC_DOWN:
		caret_move_relative(+1, 0, dir_before, select);
		break;
	case KC_HOME:
		tag_get_pt(&pane.caret_pos, &pt);
		pt_get_sol(&pt, &pt);
		caret_move(pt, select, true);
		break;
	case KC_END:
		tag_get_pt(&pane.caret_pos, &pt);
		pt_get_eol(&pt, &pt);
		caret_move(pt, select, true);
		break;
	case KC_PAGE_UP:
		caret_move_relative(-pane.rows, 0, dir_before, select);
		break;
	case KC_PAGE_DOWN:
		caret_move_relative(+pane.rows, 0, dir_before, select);
		break;
	default:
		break;
	}
}

/** Create new document. */
static errno_t file_new(void)
{
	errno_t rc;
	sheet_t *sh;

	/* Create empty sheet. */
	rc = sheet_create(&sh);
	if (rc != EOK) {
		printf("Out of memory.\n");
		return ENOMEM;
	}

	if (doc.sh != NULL)
		sheet_destroy(doc.sh);

	doc.sh = sh;

	/* Place caret at the beginning of file. */
	spt_t sof;
	pt_get_sof(&sof);
	sheet_place_tag(doc.sh, &sof, &pane.caret_pos);
	pane.ideal_column = 1;

	doc.file_name = NULL;

	/* Place selection start tag. */
	sheet_place_tag(doc.sh, &sof, &pane.sel_start);

	/* Move to beginning of file. */
	pt_get_sof(&sof);

	caret_move(sof, true, true);

	pane_status_display(&pane);
	pane_caret_display(&pane);
	pane_text_display(&pane);
	cursor_setvis(true);

	return EOK;
}

/** Open Open File dialog. */
static void file_open(void)
{
	const char *old_fname = (doc.file_name != NULL) ? doc.file_name : "";
	ui_file_dialog_params_t fdparams;
	ui_file_dialog_t *dialog;
	errno_t rc;

	ui_file_dialog_params_init(&fdparams);
	fdparams.caption = "Open File";
	fdparams.ifname = old_fname;

	rc = ui_file_dialog_create(edit.ui, &fdparams, &dialog);
	if (rc != EOK) {
		printf("Error creating message dialog.\n");
		return;
	}

	ui_file_dialog_set_cb(dialog, &open_dialog_cb, &edit);
}

/** Open exising document. */
static errno_t file_open_file(const char *fname)
{
	errno_t rc;
	sheet_t *sh;
	char *fn;

	/* Create empty sheet. */
	rc = sheet_create(&sh);
	if (rc != EOK) {
		printf("Out of memory.\n");
		return ENOMEM;
	}

	fn = str_dup(fname);
	if (fn == NULL) {
		sheet_destroy(sh);
		return ENOMEM;
	}

	if (doc.sh != NULL)
		sheet_destroy(doc.sh);

	doc.sh = sh;

	/* Place caret at the beginning of file. */
	spt_t sof;
	pt_get_sof(&sof);
	sheet_place_tag(doc.sh, &sof, &pane.caret_pos);
	pane.ideal_column = 1;

	rc = file_insert(fname);
	if (rc != EOK)
		return rc;

	doc.file_name = fn;

	/* Place selection start tag. */
	sheet_place_tag(doc.sh, &sof, &pane.sel_start);

	/* Move to beginning of file. */
	pt_get_sof(&sof);

	caret_move(sof, true, true);

	pane_status_display(&pane);
	pane_caret_display(&pane);
	pane_text_display(&pane);
	cursor_setvis(true);

	return EOK;
}

/** Save the document. */
static errno_t file_save(char const *fname)
{
	spt_t sp, ep;
	errno_t rc;

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

/** Open Save As dialog. */
static void file_save_as(void)
{
	const char *old_fname = (doc.file_name != NULL) ? doc.file_name : "";
	ui_file_dialog_params_t fdparams;
	ui_file_dialog_t *dialog;
	errno_t rc;

	ui_file_dialog_params_init(&fdparams);
	fdparams.caption = "Save As";
	fdparams.ifname = old_fname;

	rc = ui_file_dialog_create(edit.ui, &fdparams, &dialog);
	if (rc != EOK) {
		printf("Error creating message dialog.\n");
		return;
	}

	ui_file_dialog_set_cb(dialog, &save_as_dialog_cb, &edit);
}

/** Insert file at caret position.
 *
 * Reads in the contents of a file and inserts them at the current position
 * of the caret.
 */
static errno_t file_insert(const char *fname)
{
	FILE *f;
	char32_t c;
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
		memmove(buf, buf + off, bcnt);

		insert_char(c);
	}

	fclose(f);

	return EOK;
}

/** Save a range of text into a file. */
static errno_t file_save_range(char const *fname, spt_t const *spos,
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
		sheet_copy_out(doc.sh, &sp, epos, buf, BUF_SIZE, &bep);
		bytes = str_size(buf);

		n_written = fwrite(buf, 1, bytes, f);
		if (n_written != bytes) {
			return EIO;
		}

		sp = bep;
	} while (!spt_equal(&bep, epos));

	if (fclose(f) < 0)
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
		sheet_copy_out(doc.sh, &sp, epos, &buf[bpos], buf_size - bpos,
		    &bep);
		bytes = str_size(&buf[bpos]);
		bpos += bytes;
		sp = bep;

		if (spt_equal(&bep, epos))
			break;

		buf_size *= 2;
		char *tmp = realloc(buf, buf_size);
		if (tmp == NULL) {
			free(buf);
			return NULL;
		}
		buf = tmp;
	}

	return buf;
}

/** Initialize pane.
 *
 * TODO: Replace with pane_create() that allocates the pane.
 *
 * @param window Editor window
 * @param pane Pane
 * @return EOK on success or an error code
 */
static errno_t pane_init(ui_window_t *window, pane_t *pane)
{
	errno_t rc;
	gfx_rect_t arect;

	pane->control = NULL;
	pane->color = NULL;
	pane->sel_color = NULL;

	rc = ui_control_new(&pane_ctl_ops, (void *) pane, &pane->control);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x07, &pane->color);
	if (rc != EOK)
		goto error;

	rc = gfx_color_new_ega(0x1e, &pane->sel_color);
	if (rc != EOK)
		goto error;

	pane->res = ui_window_get_res(window);
	pane->window = window;

	ui_window_get_app_rect(window, &arect);
	pane->rect.p0.x = arect.p0.x;
	pane->rect.p0.y = arect.p0.y + 1;
	pane->rect.p1.x = arect.p1.x;
	pane->rect.p1.y = arect.p1.y - 1;

	pane->columns = pane->rect.p1.x - pane->rect.p0.x;
	pane->rows = pane->rect.p1.y - pane->rect.p0.y;

	return EOK;
error:
	if (pane->control != NULL) {
		ui_control_delete(pane->control);
		pane->control = NULL;
	}

	if (pane->color != NULL) {
		gfx_color_delete(pane->color);
		pane->color = NULL;
	}

	return rc;
}

/** Finalize pane.
 *
 * TODO: Replace with pane_destroy() that deallocates the pane.
 *
 * @param pane Pane
 */
static void pane_fini(pane_t *pane)
{
	gfx_color_delete(pane->color);
	pane->color = NULL;
	gfx_color_delete(pane->sel_color);
	pane->sel_color = NULL;
	ui_control_delete(pane->control);
	pane->control = NULL;
}

/** Return base control object for a pane.
 *
 * @param pane Pane
 * @return Base UI cntrol
 */
static ui_control_t *pane_ctl(pane_t *pane)
{
	return pane->control;
}

/** Repaint parts of pane that need updating.
 *
 * @param pane Pane
 * @return EOK on succes or an error code
 */
static errno_t pane_update(pane_t *pane)
{
	errno_t rc;

	if (pane->rflags & REDRAW_TEXT) {
		rc = pane_text_display(pane);
		if (rc != EOK)
			return rc;
	}

	if (pane->rflags & REDRAW_ROW)
		pane_row_display();

	if (pane->rflags & REDRAW_STATUS)
		pane_status_display(pane);

	if (pane->rflags & REDRAW_CARET)
		pane_caret_display(pane);

	pane->rflags &= ~(REDRAW_TEXT | REDRAW_ROW | REDRAW_STATUS |
	    REDRAW_CARET);
	return EOK;
}

/** Display pane text.
 *
 * @param pane Pane
 * @return EOK on success or an error code
 */
static errno_t pane_text_display(pane_t *pane)
{
	gfx_rect_t rect;
	gfx_context_t *gc;
	errno_t rc;
	int sh_rows, rows;

	sheet_get_num_rows(doc.sh, &sh_rows);
	rows = min(sh_rows - pane->sh_row + 1, pane->rows);

	/* Draw rows from the sheet. */

	rc = pane_row_range_display(pane, 0, rows);
	if (rc != EOK)
		return rc;

	/* Clear the remaining rows if file is short. */

	gc = ui_window_get_gc(pane->window);

	rc = gfx_set_color(gc, pane->color);
	if (rc != EOK)
		goto error;

	rect.p0.x = pane->rect.p0.x;
	rect.p0.y = pane->rect.p0.y + rows;
	rect.p1.x = pane->rect.p1.x;
	rect.p1.y = pane->rect.p1.y;

	rc = gfx_fill_rect(gc, &rect);
	if (rc != EOK)
		goto error;

	pane->rflags &= ~REDRAW_ROW;
	return EOK;
error:
	return rc;
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
	(void) pane_row_range_display(&pane, ridx, ridx + 1);
	pane.rflags |= (REDRAW_STATUS | REDRAW_CARET);
}

/** Display a range of rows of text.
 *
 * @param r0 Start row (inclusive)
 * @param r1 End row (exclusive)
 * @return EOk on success or an error code
 */
static errno_t pane_row_range_display(pane_t *pane, int r0, int r1)
{
	int i, fill;
	spt_t rb, re, dep, pt;
	coord_t rbc, rec;
	char row_buf[ROW_BUF_SIZE];
	char cbuf[STR_BOUNDS(1) + 1];
	char32_t c;
	size_t pos, size;
	size_t cpos;
	int s_column;
	coord_t csel_start, csel_end, ctmp;
	gfx_font_t *font;
	gfx_context_t *gc;
	gfx_text_fmt_t fmt;
	gfx_coord2_t tpos;
	gfx_rect_t rect;
	errno_t rc;

	font = ui_resource_get_font(edit.ui_res);
	gc = ui_window_get_gc(edit.window);

	gfx_text_fmt_init(&fmt);
	fmt.font = font;
	fmt.color = pane->color;

	/* Determine selection start and end. */

	tag_get_pt(&pane->sel_start, &pt);
	spt_get_coord(&pt, &csel_start);

	tag_get_pt(&pane->caret_pos, &pt);
	spt_get_coord(&pt, &csel_end);

	if (coord_cmp(&csel_start, &csel_end) > 0) {
		ctmp = csel_start;
		csel_start = csel_end;
		csel_end = ctmp;
	}

	/* Draw rows from the sheet. */

	for (i = r0; i < r1; ++i) {
		tpos.x = pane->rect.p0.x;
		tpos.y = pane->rect.p0.y + i;

		/* Starting point for row display */
		rbc.row = pane->sh_row + i;
		rbc.column = pane->sh_column;
		sheet_get_cell_pt(doc.sh, &rbc, dir_before, &rb);

		/* Ending point for row display */
		rec.row = pane->sh_row + i;
		rec.column = pane->sh_column + pane->columns;
		sheet_get_cell_pt(doc.sh, &rec, dir_before, &re);

		/* Copy the text of the row to the buffer. */
		sheet_copy_out(doc.sh, &rb, &re, row_buf, ROW_BUF_SIZE, &dep);

		/* Display text from the buffer. */

		if (coord_cmp(&csel_start, &rbc) <= 0 &&
		    coord_cmp(&rbc, &csel_end) < 0) {
			fmt.color = pane->sel_color;
		}

		size = str_size(row_buf);
		pos = 0;
		s_column = pane->sh_column;
		while (pos < size) {
			if ((csel_start.row == rbc.row) && (csel_start.column == s_column))
				fmt.color = pane->sel_color;

			if ((csel_end.row == rbc.row) && (csel_end.column == s_column))
				fmt.color = pane->color;

			c = str_decode(row_buf, &pos, size);
			if (c != '\t') {
				cpos = 0;
				rc = chr_encode(c, cbuf, &cpos, sizeof(cbuf));
				if (rc != EOK)
					return rc;

				rc = gfx_puttext(&tpos, &fmt, cbuf);
				if (rc != EOK)
					return rc;

				s_column += 1;
				tpos.x++;
			} else {
				fill = 1 + ALIGN_UP(s_column, TAB_WIDTH) -
				    s_column;

				rc = gfx_set_color(gc, fmt.color);
				if (rc != EOK)
					return rc;

				rect.p0.x = tpos.x;
				rect.p0.y = tpos.y;
				rect.p1.x = tpos.x + fill;
				rect.p1.y = tpos.y + 1;

				rc = gfx_fill_rect(gc, &rect);
				if (rc != EOK)
					return rc;

				s_column += fill;
				tpos.x += fill;
			}
		}

		if ((csel_end.row == rbc.row) && (csel_end.column == s_column))
			fmt.color = pane->color;

		/* Fill until the end of display area. */

		rc = gfx_set_color(gc, fmt.color);
		if (rc != EOK)
			return rc;

		rect.p0.x = tpos.x;
		rect.p0.y = tpos.y;
		rect.p1.x = pane->rect.p1.x;
		rect.p1.y = tpos.y + 1;

		rc = gfx_fill_rect(gc, &rect);
		if (rc != EOK)
			return rc;
	}

	return EOK;
}

/** Display pane status in the status line.
 *
 * @param pane Pane
 */
static void pane_status_display(pane_t *pane)
{
	spt_t caret_pt;
	coord_t coord;
	int last_row;
	char *fname;
	char *p;
	char *text;
	size_t n;
	size_t nextra;
	size_t fnw;

	tag_get_pt(&pane->caret_pos, &caret_pt);
	spt_get_coord(&caret_pt, &coord);

	sheet_get_num_rows(doc.sh, &last_row);

	if (doc.file_name != NULL) {
		/* Remove directory component */
		p = str_rchr(doc.file_name, '/');
		if (p != NULL)
			fname = str_dup(p + 1);
		else
			fname = str_dup(doc.file_name);
	} else {
		fname = str_dup("<unnamed>");
	}

	if (fname == NULL)
		return;

	/*
	 * Make sure the status fits on the screen. This loop should
	 * be executed at most twice.
	 */
	while (true) {
		int rc = asprintf(&text, "%d, %d (%d): File '%s'. Ctrl-Q Quit  "
		    "F10 Menu", coord.row, coord.column, last_row, fname);
		if (rc < 0) {
			n = 0;
			goto finish;
		}

		/* If it already fits, we're done */
		n = str_width(text);
		if ((int)n <= pane->columns - 2)
			break;

		/* Compute number of excess characters */
		nextra = n - (pane->columns - 2);
		/** With of the file name part */
		fnw = str_width(fname);

		/*
		 * If reducing file name to two characters '..' won't help,
		 * just give up and print a blank status.
		 */
		if (nextra > fnw - 2) {
			text[0] = '\0';
			goto finish;
		}

		/* Compute position where we overwrite with '..\0' */
		if (fnw >= nextra + 2) {
			p = fname + str_lsize(fname, fnw - nextra - 2);
		} else {
			p = fname;
		}

		/* Shorten the string */
		p[0] = p[1] = '.';
		p[2] = '\0';

		/* Need to format the string once more. */
		free(text);
	}

finish:
	(void) ui_label_set_text(edit.status, text);
	(void) ui_label_paint(edit.status);
	free(text);
	free(fname);
}

/** Set cursor to reflect position of the caret.
 *
 * @param pane Pane
 */
static void pane_caret_display(pane_t *pane)
{
	spt_t caret_pt;
	coord_t coord;
	gfx_coord2_t pos;
	gfx_context_t *gc;

	tag_get_pt(&pane->caret_pos, &caret_pt);

	spt_get_coord(&caret_pt, &coord);

	gc = ui_window_get_gc(edit.window);
	pos.x = pane->rect.p0.x + coord.column - pane->sh_column;
	pos.y = pane->rect.p0.y + coord.row - pane->sh_row;

	(void) gfx_cursor_set_pos(gc, &pos);
}

/** Destroy pane control.
 *
 * @param arg Argument (pane_t *)
 */
static void pane_ctl_destroy(void *arg)
{
	pane_t *pane = (pane_t *)arg;

	pane_fini(pane);
}

/** Paint pane control.
 *
 * @param arg Argument (pane_t *)
 */
static errno_t pane_ctl_paint(void *arg)
{
	pane_t *pane = (pane_t *)arg;
	gfx_context_t *gc;
	errno_t rc;

	gc = ui_window_get_gc(pane->window);

	rc = pane_text_display(pane);
	if (rc != EOK)
		goto error;

	rc = gfx_update(gc);
	if (rc != EOK)
		goto error;

error:
	return rc;
}

/** Handle pane control position event.
 *
 * @param arg Argument (pane_t *)
 * @param event Position event
 */
static ui_evclaim_t pane_ctl_pos_event(void *arg, pos_event_t *event)
{
	gfx_coord2_t pos;

	pos.x = event->hpos;
	pos.y = event->vpos;

	if (!gfx_pix_inside_rect(&pos, &pane.rect))
		return ui_unclaimed;

	pos_handle(event);
	(void) gfx_update(ui_window_get_gc(edit.window));
	return ui_claimed;
}

/** Insert a character at caret position. */
static void insert_char(char32_t c)
{
	spt_t pt;
	char cbuf[STR_BOUNDS(1) + 1];
	size_t offs;

	tag_get_pt(&pane.caret_pos, &pt);

	offs = 0;
	chr_encode(c, cbuf, &offs, STR_BOUNDS(1) + 1);
	cbuf[offs] = '\0';

	(void) sheet_insert(doc.sh, &pt, dir_before, cbuf);

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
	sheet_get_cell_pt(doc.sh, &coord, dir_before, &sp);

	(void) sheet_delete(doc.sh, &sp, &ep);

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

	sheet_get_cell_pt(doc.sh, &sc, dir_after, &ep);
	spt_get_coord(&ep, &ec);

	(void) sheet_delete(doc.sh, &sp, &ep);

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

/** Relatively move caret position.
 *
 * Moves caret relatively to the current position. Looking at the first
 * character cell after the caret and moving by @a drow and @a dcolumn, we get
 * to a new character cell, and thus a new character. Then we either go to the
 * point before the the character or after it, depending on @a align_dir.
 *
 * @param select true if the selection tag should stay where it is
 */
static void caret_move_relative(int drow, int dcolumn, enum dir_spec align_dir,
    bool select)
{
	spt_t pt;
	coord_t coord;
	int num_rows;
	bool pure_vertical;

	tag_get_pt(&pane.caret_pos, &pt);
	spt_get_coord(&pt, &coord);
	coord.row += drow;
	coord.column += dcolumn;

	/* Clamp coordinates. */
	if (drow < 0 && coord.row < 1)
		coord.row = 1;
	if (dcolumn < 0 && coord.column < 1) {
		if (coord.row < 2)
			coord.column = 1;
		else {
			coord.row--;
			sheet_get_row_width(doc.sh, coord.row, &coord.column);
		}
	}
	if (drow > 0) {
		sheet_get_num_rows(doc.sh, &num_rows);
		if (coord.row > num_rows)
			coord.row = num_rows;
	}

	/* For purely vertical movement try attaining @c ideal_column. */
	pure_vertical = (dcolumn == 0 && align_dir == dir_before);
	if (pure_vertical)
		coord.column = pane.ideal_column;

	/*
	 * Select the point before or after the character at the designated
	 * coordinates. The character can be wider than one cell (e.g. tab).
	 */
	sheet_get_cell_pt(doc.sh, &coord, align_dir, &pt);

	/* For non-vertical movement set the new value for @c ideal_column. */
	caret_move(pt, select, !pure_vertical);
}

/** Absolutely move caret position.
 *
 * Moves caret to a specified position. We get to a new character cell, and
 * thus a new character. Then we either go to the point before the the character
 * or after it, depending on @a align_dir.
 *
 * @param select true if the selection tag should stay where it is
 */
static void caret_move_absolute(int row, int column, enum dir_spec align_dir,
    bool select)
{
	coord_t coord;
	coord.row = row;
	coord.column = column;

	spt_t pt;
	sheet_get_cell_pt(doc.sh, &coord, align_dir, &pt);

	caret_move(pt, select, true);
}

/** Find beginning of a word to the left of spt */
static spt_t pt_find_word_left(spt_t spt)
{
	do {
		spt_prev_char(spt, &spt);
	} while (!pt_is_word_beginning(&spt));
	return spt;
}

/** Find beginning of a word to the right of spt */
static spt_t pt_find_word_right(spt_t spt)
{
	do {
		spt_next_char(spt, &spt);
	} while (!pt_is_word_beginning(&spt));
	return spt;
}

static void caret_move_word_left(bool select)
{
	spt_t pt;
	tag_get_pt(&pane.caret_pos, &pt);
	spt_t word_left = pt_find_word_left(pt);
	caret_move(word_left, select, true);
}

static void caret_move_word_right(bool select)
{
	spt_t pt;
	tag_get_pt(&pane.caret_pos, &pt);
	spt_t word_right = pt_find_word_right(pt);
	caret_move(word_right, select, true);
}

/** Ask for line and go to it. */
static void caret_go_to_line_ask(void)
{
	ui_prompt_dialog_params_t pdparams;
	ui_prompt_dialog_t *dialog;
	errno_t rc;

	ui_prompt_dialog_params_init(&pdparams);
	pdparams.caption = "Go To Line";
	pdparams.prompt = "Line Number";

	rc = ui_prompt_dialog_create(edit.ui, &pdparams, &dialog);
	if (rc != EOK) {
		printf("Error creating prompt dialog.\n");
		return;
	}

	ui_prompt_dialog_set_cb(dialog, &go_to_line_dialog_cb, &edit);
}

/* Search operations */
static errno_t search_spt_producer(void *data, char32_t *ret)
{
	assert(data != NULL);
	assert(ret != NULL);
	spt_t *spt = data;
	*ret = spt_next_char(*spt, spt);
	return EOK;
}

static errno_t search_spt_reverse_producer(void *data, char32_t *ret)
{
	assert(data != NULL);
	assert(ret != NULL);
	spt_t *spt = data;
	*ret = spt_prev_char(*spt, spt);
	return EOK;
}

static errno_t search_spt_mark(void *data, void **mark)
{
	assert(data != NULL);
	assert(mark != NULL);
	spt_t *spt = data;
	spt_t *new = calloc(1, sizeof(spt_t));
	*mark = new;
	if (new == NULL)
		return ENOMEM;
	*new = *spt;
	return EOK;
}

static void search_spt_mark_free(void *data)
{
	free(data);
}

static search_ops_t search_spt_ops = {
	.equals = char_exact_equals,
	.producer = search_spt_producer,
	.mark = search_spt_mark,
	.mark_free = search_spt_mark_free,
};

static search_ops_t search_spt_reverse_ops = {
	.equals = char_exact_equals,
	.producer = search_spt_reverse_producer,
	.mark = search_spt_mark,
	.mark_free = search_spt_mark_free,
};

/** Ask for line and go to it. */
static void search_prompt(bool reverse)
{
	ui_prompt_dialog_params_t pdparams;
	ui_prompt_dialog_t *dialog;
	errno_t rc;

	ui_prompt_dialog_params_init(&pdparams);
	pdparams.caption = reverse ? "Reverse Search" : "Search";
	pdparams.prompt = "Search text";
	pdparams.itext = "";

	if (pane.previous_search)
		pdparams.itext = pane.previous_search;

	rc = ui_prompt_dialog_create(edit.ui, &pdparams, &dialog);
	if (rc != EOK) {
		printf("Error creating prompt dialog.\n");
		return;
	}

	ui_prompt_dialog_set_cb(dialog, &search_dialog_cb, &edit);
	pane.search_reverse = reverse;
}

static void search_repeat(void)
{
	if (pane.previous_search == NULL) {
		status_display("No previous search to repeat.");
		return;
	}

	search(pane.previous_search, pane.previous_search_reverse);
}

static void search(char *pattern, bool reverse)
{
	status_display("Searching...");

	spt_t sp, producer_pos;
	tag_get_pt(&pane.caret_pos, &sp);

	/* Start searching on the position before/after caret */
	if (!reverse) {
		spt_next_char(sp, &sp);
	} else {
		spt_prev_char(sp, &sp);
	}
	producer_pos = sp;

	search_ops_t ops = search_spt_ops;
	if (reverse)
		ops = search_spt_reverse_ops;

	search_t *search = search_init(pattern, &producer_pos, ops, reverse);
	if (search == NULL) {
		status_display("Failed initializing search.");
		return;
	}

	match_t match;
	errno_t rc = search_next_match(search, &match);
	if (rc != EOK) {
		status_display("Failed searching.");
		search_fini(search);
	}

	if (match.end) {
		status_display("Match found.");
		assert(match.end != NULL);
		spt_t *end = match.end;
		caret_move(*end, false, true);
		while (match.length > 0) {
			match.length--;
			if (reverse) {
				spt_next_char(*end, end);
			} else {
				spt_prev_char(*end, end);
			}
		}
		caret_move(*end, true, true);
		free(end);
	} else {
		status_display("Not found.");
	}

	search_fini(search);
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
		sheet_delete(doc.sh, &pa, &pb);
	else
		sheet_delete(doc.sh, &pb, &pa);

	if (ca.row == cb.row)
		pane.rflags |= REDRAW_ROW;
	else
		pane.rflags |= REDRAW_TEXT;
}

/** Select all text in the editor */
static void selection_sel_all(void)
{
	spt_t spt, ept;

	pt_get_sof(&spt);
	pt_get_eof(&ept);

	selection_sel_range(spt, ept);
}

/** Select select all text in a given range with the given direction */
static void selection_sel_range(spt_t pa, spt_t pb)
{
	sheet_remove_tag(doc.sh, &pane.sel_start);
	sheet_place_tag(doc.sh, &pa, &pane.sel_start);
	sheet_remove_tag(doc.sh, &pane.caret_pos);
	sheet_place_tag(doc.sh, &pb, &pane.caret_pos);

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

static void edit_paste(void)
{
	selection_delete();
	insert_clipboard_data();
	pane.rflags |= (REDRAW_TEXT | REDRAW_CARET);
	pane_update(&pane);
}

static void edit_cut(void)
{
	selection_copy();
	selection_delete();
	pane.rflags |= (REDRAW_TEXT | REDRAW_CARET);
	pane_update(&pane);
}

static void insert_clipboard_data(void)
{
	char *str;
	size_t off;
	char32_t c;
	errno_t rc;

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
	sheet_get_cell_pt(doc.sh, &coord, dir_before, pt);
}

/** Get end-of-file s-point. */
static void pt_get_eof(spt_t *pt)
{
	coord_t coord;
	int num_rows;

	sheet_get_num_rows(doc.sh, &num_rows);
	coord.row = num_rows + 1;
	coord.column = 1;

	sheet_get_cell_pt(doc.sh, &coord, dir_after, pt);
}

/** Get start-of-line s-point for given s-point cpt */
static void pt_get_sol(spt_t *cpt, spt_t *spt)
{
	coord_t coord;

	spt_get_coord(cpt, &coord);
	coord.column = 1;

	sheet_get_cell_pt(doc.sh, &coord, dir_before, spt);
}

/** Get end-of-line s-point for given s-point cpt */
static void pt_get_eol(spt_t *cpt, spt_t *ept)
{
	coord_t coord;
	int row_width;

	spt_get_coord(cpt, &coord);
	sheet_get_row_width(doc.sh, coord.row, &row_width);
	coord.column = row_width - 1;

	sheet_get_cell_pt(doc.sh, &coord, dir_after, ept);
}

/** Check whether the spt is at a beginning of a word */
static bool pt_is_word_beginning(spt_t *pt)
{
	spt_t lp, sfp, efp, slp, elp;
	coord_t coord;

	pt_get_sof(&sfp);
	pt_get_eof(&efp);
	pt_get_sol(pt, &slp);
	pt_get_eol(pt, &elp);

	/* the spt is at the beginning or end of the file or line */
	if ((spt_cmp(&sfp, pt) == 0) || (spt_cmp(&efp, pt) == 0) ||
	    (spt_cmp(&slp, pt) == 0) || (spt_cmp(&elp, pt) == 0))
		return true;

	/* the spt is a delimiter */
	if (pt_is_delimiter(pt))
		return false;

	spt_get_coord(pt, &coord);

	coord.column -= 1;
	sheet_get_cell_pt(doc.sh, &coord, dir_before, &lp);

	return pt_is_delimiter(&lp) ||
	    (pt_is_punctuation(pt) && !pt_is_punctuation(&lp)) ||
	    (pt_is_punctuation(&lp) && !pt_is_punctuation(pt));
}

static char32_t get_first_wchar(const char *str)
{
	size_t offset = 0;
	return str_decode(str, &offset, str_size(str));
}

static bool pt_is_delimiter(spt_t *pt)
{
	spt_t rp;
	coord_t coord;
	char *ch = NULL;

	spt_get_coord(pt, &coord);

	coord.column += 1;
	sheet_get_cell_pt(doc.sh, &coord, dir_after, &rp);

	ch = range_get_str(pt, &rp);
	if (ch == NULL)
		return false;

	char32_t first_char = get_first_wchar(ch);
	switch (first_char) {
	case ' ':
	case '\t':
	case '\n':
		return true;
	default:
		return false;
	}
}

static bool pt_is_punctuation(spt_t *pt)
{
	spt_t rp;
	coord_t coord;
	char *ch = NULL;

	spt_get_coord(pt, &coord);

	coord.column += 1;
	sheet_get_cell_pt(doc.sh, &coord, dir_after, &rp);

	ch = range_get_str(pt, &rp);
	if (ch == NULL)
		return false;

	char32_t first_char = get_first_wchar(ch);
	switch (first_char) {
	case ',':
	case '.':
	case ';':
	case ':':
	case '/':
	case '?':
	case '\\':
	case '|':
	case '_':
	case '+':
	case '-':
	case '*':
	case '=':
	case '<':
	case '>':
		return true;
	default:
		return false;
	}
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
	(void) ui_label_set_text(edit.status, str);
	(void) ui_label_paint(edit.status);
}

/** Window close request
 *
 * @param window Window
 * @param arg Argument (edit_t *)
 */
static void edit_wnd_close(ui_window_t *window, void *arg)
{
	edit_t *edit = (edit_t *) arg;

	ui_quit(edit->ui);
}

/** Window focus event
 *
 * @param window Window
 * @param arg Argument (edit_t *)
 * @param focus Focus number
 */
static void edit_wnd_focus(ui_window_t *window, void *arg, unsigned focus)
{
	edit_t *edit = (edit_t *)arg;

	(void)edit;
	pane_caret_display(&pane);
	cursor_setvis(true);
}

/** Window keyboard event
 *
 * @param window Window
 * @param arg Argument (edit_t *)
 * @param event Keyboard event
 */
static void edit_wnd_kbd_event(ui_window_t *window, void *arg,
    kbd_event_t *event)
{
	pane.keymod = event->mods;

	if (ui_window_def_kbd(window, event) == ui_claimed)
		return;

	if (event->type == KEY_PRESS) {
		key_handle_press(event);
		(void) pane_update(&pane);
		(void) gfx_update(ui_window_get_gc(window));
	}
}

/** Window unfocus event
 *
 * @param window Window
 * @param arg Argument (edit_t *)
 * @param focus Focus number
 */
static void edit_wnd_unfocus(ui_window_t *window, void *arg, unsigned focus)
{
	edit_t *edit = (edit_t *) arg;

	(void)edit;
	cursor_setvis(false);
}

/** Menu bar activate event
 *
 * @param mbar Menu bar
 * @param arg Argument (edit_t *)
 */
static void edit_menubar_activate(ui_menu_bar_t *mbar, void *arg)
{
	edit_t *edit = (edit_t *)arg;

	(void)edit;
	cursor_setvis(false);
}

/** Menu bar deactivate event
 *
 * @param mbar Menu bar
 * @param arg Argument (edit_t *)
 */
static void edit_menubar_deactivate(ui_menu_bar_t *mbar, void *arg)
{
	edit_t *edit = (edit_t *)arg;

	(void)edit;
	pane_caret_display(&pane);
	cursor_setvis(true);
}

/** File / New menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (edit_t *)
 */
static void edit_file_new(ui_menu_entry_t *mentry, void *arg)
{
	edit_t *edit = (edit_t *) arg;

	(void)edit;
	file_new();
	(void) gfx_update(ui_window_get_gc(edit->window));
}

/** File / Open menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (edit_t *)
 */
static void edit_file_open(ui_menu_entry_t *mentry, void *arg)
{
	edit_t *edit = (edit_t *) arg;

	(void)edit;
	file_open();
}

/** File / Save menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (edit_t *)
 */
static void edit_file_save(ui_menu_entry_t *mentry, void *arg)
{
	edit_t *edit = (edit_t *) arg;

	(void)edit;

	if (doc.file_name != NULL)
		file_save(doc.file_name);
	else
		file_save_as();
}

/** File / Save As menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (edit_t *)
 */
static void edit_file_save_as(ui_menu_entry_t *mentry, void *arg)
{
	edit_t *edit = (edit_t *) arg;

	(void)edit;
	file_save_as();
}

/** File / Exit menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (edit_t *)
 */
static void edit_file_exit(ui_menu_entry_t *mentry, void *arg)
{
	edit_t *edit = (edit_t *) arg;

	ui_quit(edit->ui);
}

/** Edit / Cut menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (edit_t *)
 */
static void edit_edit_cut(ui_menu_entry_t *mentry, void *arg)
{
	(void) arg;
	edit_cut();
	(void) gfx_update(ui_window_get_gc(edit.window));
}

/** Edit / Copy menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (edit_t *)
 */
static void edit_edit_copy(ui_menu_entry_t *mentry, void *arg)
{
	(void) arg;
	selection_copy();
}

/** Edit / Paste menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (edit_t *)
 */
static void edit_edit_paste(ui_menu_entry_t *mentry, void *arg)
{
	(void) arg;
	edit_paste();
	(void) gfx_update(ui_window_get_gc(edit.window));
}

/** Edit / Delete menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (edit_t *)
 */
static void edit_edit_delete(ui_menu_entry_t *mentry, void *arg)
{
	(void) arg;

	if (selection_active())
		selection_delete();

	pane.rflags |= REDRAW_CARET;
	(void) pane_update(&pane);
	(void) gfx_update(ui_window_get_gc(edit.window));
}

/** Edit / Select All menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (edit_t *)
 */
static void edit_edit_select_all(ui_menu_entry_t *mentry, void *arg)
{
	(void) arg;

	selection_sel_all();
	pane.rflags |= (REDRAW_CARET | REDRAW_TEXT | REDRAW_STATUS);
	pane_update(&pane);
	(void) gfx_update(ui_window_get_gc(edit.window));
}

/** Search / Find menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (edit_t *)
 */
static void edit_search_find(ui_menu_entry_t *mentry, void *arg)
{
	(void) arg;
	search_prompt(false);
}

/** Search / Reverse Find menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (edit_t *)
 */
static void edit_search_reverse_find(ui_menu_entry_t *mentry, void *arg)
{
	(void) arg;
	search_prompt(true);
}

/** Search / Find Next menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (edit_t *)
 */
static void edit_search_find_next(ui_menu_entry_t *mentry, void *arg)
{
	(void) arg;
	search_repeat();
	(void) pane_update(&pane);
	(void) gfx_update(ui_window_get_gc(edit.window));
}

/** Search / Go To Line menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (edit_t *)
 */
static void edit_search_go_to_line(ui_menu_entry_t *mentry, void *arg)
{
	(void) arg;
	caret_go_to_line_ask();
}

/** Open File dialog OK button press.
 *
 * @param dialog Open File dialog
 * @param arg Argument (ui_demo_t *)
 * @param fname File name
 */
static void open_dialog_bok(ui_file_dialog_t *dialog, void *arg,
    const char *fname)
{
	edit_t *edit = (edit_t *)arg;
	char *cname;
	errno_t rc;

	(void)edit;
	ui_file_dialog_destroy(dialog);

	cname = str_dup(fname);
	if (cname == NULL) {
		printf("Out of memory.\n");
		return;
	}

	rc = file_open_file(fname);
	if (rc != EOK)
		return;

	if (doc.file_name != NULL)
		free(doc.file_name);
	doc.file_name = cname;

	(void) gfx_update(ui_window_get_gc(edit->window));
}

/** Open File dialog cancel button press.
 *
 * @param dialog File dialog
 * @param arg Argument (ui_demo_t *)
 */
static void open_dialog_bcancel(ui_file_dialog_t *dialog, void *arg)
{
	edit_t *edit = (edit_t *)arg;

	(void)edit;
	ui_file_dialog_destroy(dialog);
}

/** Open File dialog close request.
 *
 * @param dialog File dialog
 * @param arg Argument (ui_demo_t *)
 */
static void open_dialog_close(ui_file_dialog_t *dialog, void *arg)
{
	edit_t *edit = (edit_t *)arg;

	(void)edit;
	ui_file_dialog_destroy(dialog);
}

/** Save As dialog OK button press.
 *
 * @param dialog Save As dialog
 * @param arg Argument (ui_demo_t *)
 * @param fname File name
 */
static void save_as_dialog_bok(ui_file_dialog_t *dialog, void *arg,
    const char *fname)
{
	edit_t *edit = (edit_t *)arg;
	char *cname;
	errno_t rc;

	(void)edit;
	ui_file_dialog_destroy(dialog);

	cname = str_dup(fname);
	if (cname == NULL) {
		printf("Out of memory.\n");
		return;
	}

	rc = file_save(fname);
	if (rc != EOK)
		return;

	if (doc.file_name != NULL)
		free(doc.file_name);
	doc.file_name = cname;

}

/** Save As dialog cancel button press.
 *
 * @param dialog File dialog
 * @param arg Argument (ui_demo_t *)
 */
static void save_as_dialog_bcancel(ui_file_dialog_t *dialog, void *arg)
{
	edit_t *edit = (edit_t *)arg;

	(void)edit;
	ui_file_dialog_destroy(dialog);
}

/** Save As dialog close request.
 *
 * @param dialog File dialog
 * @param arg Argument (ui_demo_t *)
 */
static void save_as_dialog_close(ui_file_dialog_t *dialog, void *arg)
{
	edit_t *edit = (edit_t *)arg;

	(void)edit;
	ui_file_dialog_destroy(dialog);
}

/** Go To Line dialog OK button press.
 *
 * @param dialog Go To Line dialog
 * @param arg Argument (ui_demo_t *)
 * @param text Submitted text
 */
static void go_to_line_dialog_bok(ui_prompt_dialog_t *dialog, void *arg,
    const char *text)
{
	edit_t *edit = (edit_t *) arg;
	char *endptr;
	int line;

	ui_prompt_dialog_destroy(dialog);
	line = strtol(text, &endptr, 10);
	if (*endptr != '\0') {
		status_display("Invalid number entered.");
		return;
	}

	caret_move_absolute(line, pane.ideal_column, dir_before, false);
	(void)edit;
	(void) pane_update(&pane);
}

/** Go To Line dialog cancel button press.
 *
 * @param dialog File dialog
 * @param arg Argument (ui_demo_t *)
 */
static void go_to_line_dialog_bcancel(ui_prompt_dialog_t *dialog, void *arg)
{
	edit_t *edit = (edit_t *) arg;

	(void)edit;
	ui_prompt_dialog_destroy(dialog);
}

/** Go To Line dialog close request.
 *
 * @param dialog File dialog
 * @param arg Argument (ui_demo_t *)
 */
static void go_to_line_dialog_close(ui_prompt_dialog_t *dialog, void *arg)
{
	edit_t *edit = (edit_t *) arg;

	(void)edit;
	ui_prompt_dialog_destroy(dialog);
}

/** Search dialog OK button press.
 *
 * @param dialog Search dialog
 * @param arg Argument (ui_demo_t *)
 * @param text Submitted text
 */
static void search_dialog_bok(ui_prompt_dialog_t *dialog, void *arg,
    const char *text)
{
	edit_t *edit = (edit_t *) arg;
	char *pattern;
	bool reverse;

	(void)edit;
	ui_prompt_dialog_destroy(dialog);

	/* Abort if search phrase is empty */
	if (text[0] == '\0')
		return;

	pattern = str_dup(text);
	reverse = pane.search_reverse;

	if (pane.previous_search)
		free(pane.previous_search);
	pane.previous_search = pattern;
	pane.previous_search_reverse = reverse;

	search(pattern, reverse);

	(void) pane_update(&pane);
}

/** Search dialog cancel button press.
 *
 * @param dialog File dialog
 * @param arg Argument (ui_demo_t *)
 */
static void search_dialog_bcancel(ui_prompt_dialog_t *dialog, void *arg)
{
	edit_t *edit = (edit_t *) arg;

	(void)edit;
	ui_prompt_dialog_destroy(dialog);
}

/** Search dialog close request.
 *
 * @param dialog File dialog
 * @param arg Argument (ui_demo_t *)
 */
static void search_dialog_close(ui_prompt_dialog_t *dialog, void *arg)
{
	edit_t *edit = (edit_t *) arg;

	(void)edit;
	ui_prompt_dialog_destroy(dialog);
}

/** @}
 */
