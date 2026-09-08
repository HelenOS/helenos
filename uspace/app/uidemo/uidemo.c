/*
 * Copyright (c) 2026 Jiri Svoboda
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

/** @addtogroup uidemo
 * @{
 */
/** @file User interface demo
 */

#include <fibril_synch.h>
#include <gfx/bitmap.h>
#include <gfx/coord.h>
#include <io/pixelmap.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <ui/checkbox.h>
#include <ui/entry.h>
#include <ui/filedialog.h>
#include <ui/fixed.h>
#include <ui/image.h>
#include <ui/label.h>
#include <ui/list.h>
#include <ui/menu.h>
#include <ui/menubar.h>
#include <ui/menudd.h>
#include <ui/menuentry.h>
#include <ui/msgdialog.h>
#include <ui/pbutton.h>
#include <ui/promptdialog.h>
#include <ui/resource.h>
#include <ui/scrollbar.h>
#include <ui/selectdialog.h>
#include <ui/slider.h>
#include <ui/tab.h>
#include <ui/tabset.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "uidemo.h"

enum {
	scrollbar_update_interval_ms = 1000,
	ui_demo_progress_step = 17
};

static errno_t bitmap_moire(gfx_bitmap_t *, gfx_coord_t, gfx_coord_t);

static void wnd_resize(ui_window_t *, void *);
static void wnd_close(ui_window_t *, void *);

static ui_window_cb_t window_cb = {
	.resize = wnd_resize,
	.close = wnd_close
};

static void pb_clicked(ui_pbutton_t *, void *);

static ui_pbutton_cb_t pbutton_cb = {
	.clicked = pb_clicked
};

static void checkbox_switched(ui_checkbox_t *, void *, bool);

static ui_checkbox_cb_t checkbox_cb = {
	.switched = checkbox_switched
};

static void rb_selected(ui_rbutton_group_t *, void *, void *);

static ui_rbutton_group_cb_t rbutton_group_cb = {
	.selected = rb_selected
};

static void slider_moved(ui_slider_t *, void *, gfx_coord_t);

static ui_slider_cb_t slider_cb = {
	.moved = slider_moved
};

static void scrollbar_up(ui_scrollbar_t *, void *);
static void scrollbar_down(ui_scrollbar_t *, void *);
static void scrollbar_page_up(ui_scrollbar_t *, void *);
static void scrollbar_page_down(ui_scrollbar_t *, void *);
static void scrollbar_moved(ui_scrollbar_t *, void *, gfx_coord_t);

static ui_scrollbar_cb_t scrollbar_cb = {
	.up = scrollbar_up,
	.down = scrollbar_down,
	.page_up = scrollbar_page_up,
	.page_down = scrollbar_page_down,
	.moved = scrollbar_moved
};

static void uidemo_file_load(ui_menu_entry_t *, void *);
static void uidemo_file_message(ui_menu_entry_t *, void *);
static void uidemo_file_confirmation(ui_menu_entry_t *, void *);
static void uidemo_file_exit(ui_menu_entry_t *, void *);
static void uidemo_edit_modify(ui_menu_entry_t *, void *);
static void uidemo_edit_insert_character(ui_menu_entry_t *, void *);

static void file_dialog_bok(ui_file_dialog_t *, void *, const char *);
static void file_dialog_bcancel(ui_file_dialog_t *, void *);
static void file_dialog_close(ui_file_dialog_t *, void *);

static ui_file_dialog_cb_t file_dialog_cb = {
	.bok = file_dialog_bok,
	.bcancel = file_dialog_bcancel,
	.close = file_dialog_close
};

static void prompt_dialog_bok(ui_prompt_dialog_t *, void *, const char *);
static void prompt_dialog_bcancel(ui_prompt_dialog_t *, void *);
static void prompt_dialog_close(ui_prompt_dialog_t *, void *);

static ui_prompt_dialog_cb_t prompt_dialog_cb = {
	.bok = prompt_dialog_bok,
	.bcancel = prompt_dialog_bcancel,
	.close = prompt_dialog_close
};

static void select_dialog_bok(ui_select_dialog_t *, void *, void *);
static void select_dialog_bcancel(ui_select_dialog_t *, void *);
static void select_dialog_close(ui_select_dialog_t *, void *);

static ui_select_dialog_cb_t select_dialog_cb = {
	.bok = select_dialog_bok,
	.bcancel = select_dialog_bcancel,
	.close = select_dialog_close
};

static void msg_dialog_button(ui_msg_dialog_t *, void *, unsigned);
static void msg_dialog_close(ui_msg_dialog_t *, void *);

static ui_msg_dialog_cb_t msg_dialog_cb = {
	.button = msg_dialog_button,
	.close = msg_dialog_close
};

static void ui_demo_destroy(ui_demo_t *);
static void ui_demo_get_geom(ui_demo_t *, gfx_rect_t *, ui_demo_geom_t *);
static errno_t ui_demo_create_bitmap(ui_demo_t *, ui_demo_geom_t *,
    gfx_rect_t *, gfx_bitmap_t **);

/** Horizontal alignment selected by each radio button */
static const gfx_halign_t uidemo_halign[3] = {
	gfx_halign_left,
	gfx_halign_center,
	gfx_halign_right
};

/** Window has been resized.
 *
 * @param window Window
 * @param arg Argument (demo)
 */
static void wnd_resize(ui_window_t *window, void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;
	gfx_bitmap_t *bitmap;
	gfx_rect_t arect;
	ui_demo_geom_t geom;
	gfx_rect_t rect;
	errno_t rc;

	/* Get new window application rectangle. */
	ui_window_get_app_rect(window, &arect);

	/* Compute geometry. */
	ui_demo_get_geom(demo, &arect, &geom);

	rc = ui_demo_create_bitmap(demo, &geom, &rect, &bitmap);
	if (rc != EOK)
		return;

	ui_image_set_bmp(demo->image, bitmap, &rect);
	gfx_bitmap_destroy(demo->bitmap);
	demo->bitmap = bitmap;

	ui_image_set_rect(demo->image, &geom.image_rect);

	ui_menu_bar_set_rect(demo->mbar, &geom.mbar_rect);
	ui_tab_set_set_rect(demo->tabset, &geom.tabset_rect);
	ui_entry_set_rect(demo->entry, &geom.entry_rect);
	ui_label_set_rect(demo->label, &geom.label_rect);
	ui_pbutton_set_rect(demo->pb1, &geom.pb1_rect);
	ui_pbutton_set_rect(demo->pb2, &geom.pb2_rect);
	ui_checkbox_set_rect(demo->checkbox, &geom.checkbox_rect);
	ui_rbutton_set_rect(demo->rbleft, &geom.rbleft_rect);
	ui_rbutton_set_rect(demo->rbcenter, &geom.rbcenter_rect);
	ui_rbutton_set_rect(demo->rbright, &geom.rbright_rect);
	ui_slider_set_rect(demo->slider, &geom.slider_rect);
	ui_scrollbar_set_rect(demo->hscrollbar, &geom.hscrollbar_rect);
	ui_scrollbar_set_rect(demo->vscrollbar, &geom.vscrollbar_rect);
	ui_progress_set_rect(demo->progress, &geom.progress_rect);
	ui_list_set_rect(demo->list, &geom.list_rect);

	(void)ui_window_paint(window);
}

/** Window close button was clicked.
 *
 * @param window Window
 * @param arg Argument (demo)
 */
static void wnd_close(ui_window_t *window, void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	ui_quit(demo->ui);
}

/** Push button was clicked.
 *
 * @param pbutton Push button
 * @param arg Argument (demo)
 */
static void pb_clicked(ui_pbutton_t *pbutton, void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;
	errno_t rc;

	if (pbutton == demo->pb1) {
		rc = ui_entry_set_text(demo->entry, "OK pressed");
		if (rc != EOK)
			printf("Error changing entry text.\n");
	} else {
		rc = ui_entry_set_text(demo->entry, "Cancel pressed");
		if (rc != EOK)
			printf("Error changing entry text.\n");
	}
}

/** Check box was switched.
 *
 * @param checkbox Check box
 * @param arg Argument (demo)
 */
static void checkbox_switched(ui_checkbox_t *checkbox, void *arg, bool enable)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	ui_entry_set_read_only(demo->entry, enable);
}

/** Radio button was selected.
 *
 * @param rbgroup Radio button group
 * @param garg Group argument (demo)
 * @param barg Button argument
 */
static void rb_selected(ui_rbutton_group_t *rbgroup, void *garg, void *barg)
{
	ui_demo_t *demo = (ui_demo_t *) garg;
	gfx_halign_t halign = *(gfx_halign_t *) barg;

	ui_entry_set_halign(demo->entry, halign);
	(void) ui_entry_paint(demo->entry);
}

/** Slider was moved.
 *
 * @param slider Slider
 * @param arg Argument (demo)
 * @param pos Position
 */
static void slider_moved(ui_slider_t *slider, void *arg, gfx_coord_t pos)
{
	ui_demo_t *demo = (ui_demo_t *) arg;
	char *str;
	errno_t rc;
	int rv;

	rv = asprintf(&str, "Slider at %d of %d", (int) pos,
	    ui_slider_length(slider));
	if (rv < 0) {
		printf("Out of memory.\n");
		return;
	}

	rc = ui_entry_set_text(demo->entry, str);
	if (rc != EOK)
		printf("Error changing entry text.\n");
	(void) ui_entry_paint(demo->entry);

	free(str);
}

/** Scrollbar up button pressed.
 *
 * @param scrollbar Scrollbar
 * @param arg Argument (demo)
 */
static void scrollbar_up(ui_scrollbar_t *scrollbar, void *arg)
{
	gfx_coord_t pos;

	pos = ui_scrollbar_get_pos(scrollbar);
	ui_scrollbar_set_pos(scrollbar, pos - 1);

	pos = ui_scrollbar_get_pos(scrollbar);
	scrollbar_moved(scrollbar, arg, pos);
}

/** Scrollbar down button pressed.
 *
 * @param scrollbar Scrollbar
 * @param arg Argument (demo)
 */
static void scrollbar_down(ui_scrollbar_t *scrollbar, void *arg)
{
	gfx_coord_t pos;

	pos = ui_scrollbar_get_pos(scrollbar);
	ui_scrollbar_set_pos(scrollbar, pos + 1);

	pos = ui_scrollbar_get_pos(scrollbar);
	scrollbar_moved(scrollbar, arg, pos);
}

/** Scrollbar page up event.
 *
 * @param scrollbar Scrollbar
 * @param arg Argument (demo)
 */
static void scrollbar_page_up(ui_scrollbar_t *scrollbar, void *arg)
{
	gfx_coord_t pos;

	pos = ui_scrollbar_get_pos(scrollbar);
	ui_scrollbar_set_pos(scrollbar, pos -
	    ui_scrollbar_trough_length(scrollbar) / 4);

	pos = ui_scrollbar_get_pos(scrollbar);
	scrollbar_moved(scrollbar, arg, pos);
}

/** Scrollbar page down event.
 *
 * @param scrollbar Scrollbar
 * @param arg Argument (demo)
 */
static void scrollbar_page_down(ui_scrollbar_t *scrollbar, void *arg)
{
	gfx_coord_t pos;

	pos = ui_scrollbar_get_pos(scrollbar);
	ui_scrollbar_set_pos(scrollbar, pos +
	    ui_scrollbar_trough_length(scrollbar) / 4);

	pos = ui_scrollbar_get_pos(scrollbar);
	scrollbar_moved(scrollbar, arg, pos);
}

/** Scrollbar was moved.
 *
 * @param scrollbar Scrollbar
 * @param arg Argument (demo)
 * @param pos Position
 */
static void scrollbar_moved(ui_scrollbar_t *scrollbar, void *arg,
    gfx_coord_t pos)
{
	ui_demo_t *demo = (ui_demo_t *) arg;
	char *str;
	errno_t rc;
	int rv;

	rv = asprintf(&str, "Scrollbar: %d of %d", (int) pos,
	    ui_scrollbar_move_length(scrollbar));
	if (rv < 0) {
		printf("Out of memory.\n");
		return;
	}

	rc = ui_entry_set_text(demo->entry, str);
	if (rc != EOK)
		printf("Error changing entry text.\n");
	(void) ui_entry_paint(demo->entry);

	free(str);
}

/** Display a message window with OK button.
 *
 * @param demo UI demo
 * @param caption Window caption
 * @param text Message text
 */
static void uidemo_show_message(ui_demo_t *demo, const char *caption,
    const char *text)
{
	ui_msg_dialog_params_t mdparams;
	ui_msg_dialog_t *dialog;
	errno_t rc;

	ui_msg_dialog_params_init(&mdparams);
	mdparams.caption = caption;
	mdparams.text = text;

	rc = ui_msg_dialog_create(demo->ui, &mdparams, &dialog);
	if (rc != EOK) {
		printf("Error creating message dialog.\n");
		return;
	}

	ui_msg_dialog_set_cb(dialog, &msg_dialog_cb, demo);
}

/** File / Load menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (demo)
 */
static void uidemo_file_load(ui_menu_entry_t *mentry, void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;
	ui_file_dialog_params_t fdparams;
	ui_file_dialog_t *dialog;
	errno_t rc;

	ui_file_dialog_params_init(&fdparams);
	fdparams.caption = "Load File";

	rc = ui_file_dialog_create(demo->ui, &fdparams, &dialog);
	if (rc != EOK) {
		printf("Error creating message dialog.\n");
		return;
	}

	ui_file_dialog_set_cb(dialog, &file_dialog_cb, demo);
}

/** File / Message menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (demo)
 */
static void uidemo_file_message(ui_menu_entry_t *mentry, void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;
	ui_msg_dialog_params_t mdparams;
	ui_msg_dialog_t *dialog;
	errno_t rc;

	ui_msg_dialog_params_init(&mdparams);
	mdparams.caption = "Message For You";
	mdparams.text = "Hello, world!";

	rc = ui_msg_dialog_create(demo->ui, &mdparams, &dialog);
	if (rc != EOK) {
		printf("Error creating message dialog.\n");
		return;
	}

	ui_msg_dialog_set_cb(dialog, &msg_dialog_cb, demo);
}

/** File / Confirmation menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (demo)
 */
static void uidemo_file_confirmation(ui_menu_entry_t *mentry, void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;
	ui_msg_dialog_params_t mdparams;
	ui_msg_dialog_t *dialog;
	errno_t rc;

	ui_msg_dialog_params_init(&mdparams);
	mdparams.caption = "Confirmation";
	mdparams.text = "This will not actually do anything. Proceed?";
	mdparams.choice = umdc_ok_cancel;

	rc = ui_msg_dialog_create(demo->ui, &mdparams, &dialog);
	if (rc != EOK) {
		printf("Error creating message dialog.\n");
		return;
	}

	ui_msg_dialog_set_cb(dialog, &msg_dialog_cb, demo);
}

/** File / Exit menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (demo)
 */
static void uidemo_file_exit(ui_menu_entry_t *mentry, void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	ui_quit(demo->ui);
}

/** Edit / Modify menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (demo)
 */
static void uidemo_edit_modify(ui_menu_entry_t *mentry, void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;
	ui_prompt_dialog_params_t pdparams;
	ui_prompt_dialog_t *dialog;
	errno_t rc;

	ui_prompt_dialog_params_init(&pdparams);
	pdparams.caption = "Modify Entry Text";
	pdparams.prompt = "Enter New Text";

	rc = ui_prompt_dialog_create(demo->ui, &pdparams, &dialog);
	if (rc != EOK) {
		printf("Error creating prompt dialog.\n");
		return;
	}

	ui_prompt_dialog_set_cb(dialog, &prompt_dialog_cb, demo);
}

/** Edit / Insert Character menu entry selected.
 *
 * @param mentry Menu entry
 * @param arg Argument (demo)
 */
static void uidemo_edit_insert_character(ui_menu_entry_t *mentry, void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;
	ui_select_dialog_params_t sdparams;
	ui_select_dialog_t *dialog;
	ui_list_entry_attr_t attr;
	errno_t rc;

	ui_select_dialog_params_init(&sdparams);
	sdparams.caption = "Insert Character";
	sdparams.prompt = "Select character to insert";

	rc = ui_select_dialog_create(demo->ui, &sdparams, &dialog);
	if (rc != EOK) {
		printf("Error creating select dialog.\n");
		return;
	}

	ui_list_entry_attr_init(&attr);
	attr.caption = "Dollar sign ($)";
	attr.arg = (void *)'$';
	rc = ui_select_dialog_append(dialog, &attr);
	if (rc != EOK) {
		printf("Error appending entry to list.\n");
		return;
	}

	ui_list_entry_attr_init(&attr);
	attr.caption = "Hash sign (#)";
	attr.arg = (void *)'#';
	rc = ui_select_dialog_append(dialog, &attr);
	if (rc != EOK) {
		printf("Error appending entry to list.\n");
		return;
	}

	ui_list_entry_attr_init(&attr);
	attr.caption = "Question mark (?)";
	attr.arg = (void *)'?';
	rc = ui_select_dialog_append(dialog, &attr);
	if (rc != EOK) {
		printf("Error appending entry to list.\n");
		return;
	}

	ui_select_dialog_set_cb(dialog, &select_dialog_cb, demo);

	(void) ui_select_dialog_paint(dialog);
}

/** File dialog OK button press.
 *
 * @param dialog File dialog
 * @param arg Argument (ui_demo_t *)
 * @param fname File name
 */
static void file_dialog_bok(ui_file_dialog_t *dialog, void *arg,
    const char *fname)
{
	ui_demo_t *demo = (ui_demo_t *) arg;
	char buf[128];
	char *p;
	FILE *f;

	ui_file_dialog_destroy(dialog);

	f = fopen(fname, "rt");
	if (f == NULL) {
		uidemo_show_message(demo, "Error", "Error opening file.");
		return;
	}

	p = fgets(buf, sizeof(buf), f);
	if (p == NULL) {
		uidemo_show_message(demo, "Error", "Error reading file.");
		fclose(f);
		return;
	}

	/* Cut string off at the first non-printable character */
	p = buf;
	while (*p != '\0') {
		if (*p < ' ') {
			*p = '\0';
			break;
		}
		++p;
	}

	ui_entry_set_text(demo->entry, buf);
	fclose(f);
}

/** File dialog cancel button press.
 *
 * @param dialog File dialog
 * @param arg Argument (ui_demo_t *)
 */
static void file_dialog_bcancel(ui_file_dialog_t *dialog, void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	(void) demo;
	ui_file_dialog_destroy(dialog);
}

/** File dialog close request.
 *
 * @param dialog File dialog
 * @param arg Argument (ui_demo_t *)
 */
static void file_dialog_close(ui_file_dialog_t *dialog, void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	(void) demo;
	ui_file_dialog_destroy(dialog);
}

/** Prompt dialog OK button press.
 *
 * @param dialog Prompt dialog
 * @param arg Argument (ui_demo_t *)
 * @param text Submitted text
 */
static void prompt_dialog_bok(ui_prompt_dialog_t *dialog, void *arg,
    const char *text)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	ui_prompt_dialog_destroy(dialog);
	ui_entry_set_text(demo->entry, text);
}

/** Prompt dialog cancel button press.
 *
 * @param dialog Prompt dialog
 * @param arg Argument (ui_demo_t *)
 */
static void prompt_dialog_bcancel(ui_prompt_dialog_t *dialog, void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	(void) demo;
	ui_prompt_dialog_destroy(dialog);
}

/** Prompt dialog close request.
 *
 * @param dialog Prompt dialog
 * @param arg Argument (ui_demo_t *)
 */
static void prompt_dialog_close(ui_prompt_dialog_t *dialog, void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	(void) demo;
	ui_prompt_dialog_destroy(dialog);
}

/** Select dialog OK button press.
 *
 * @param dialog Select dialog
 * @param arg Argument (ui_demo_t *)
 * @param text Submitted text
 */
static void select_dialog_bok(ui_select_dialog_t *dialog, void *arg,
    void *earg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;
	char str[2];

	ui_select_dialog_destroy(dialog);
	str[0] = (char)(intptr_t)earg;
	str[1] = '\0';
	(void) ui_entry_insert_str(demo->entry, str);
}

/** Select dialog cancel button press.
 *
 * @param dialog Select dialog
 * @param arg Argument (ui_demo_t *)
 */
static void select_dialog_bcancel(ui_select_dialog_t *dialog, void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	(void) demo;
	ui_select_dialog_destroy(dialog);
}

/** Select dialog close request.
 *
 * @param dialog Select dialog
 * @param arg Argument (ui_demo_t *)
 */
static void select_dialog_close(ui_select_dialog_t *dialog, void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	(void) demo;
	ui_select_dialog_destroy(dialog);
}

/** Message dialog button press.
 *
 * @param dialog Message dialog
 * @param arg Argument (ui_demo_t *)
 * @param bnum Button number
 */
static void msg_dialog_button(ui_msg_dialog_t *dialog, void *arg,
    unsigned bnum)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	(void) demo;
	ui_msg_dialog_destroy(dialog);
}

/** Message dialog close request.
 *
 * @param dialog Message dialog
 * @param arg Argument (ui_demo_t *)
 */
static void msg_dialog_close(ui_msg_dialog_t *dialog, void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	(void) demo;
	ui_msg_dialog_destroy(dialog);
}

static void ui_demo_timer_fun(void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	if (demo->progress_value < 100) {
		demo->progress_value += ui_demo_progress_step;
		if (demo->progress_value > 100)
			demo->progress_value = 100;
	} else {
		demo->progress_value = 0;
	}

	ui_progress_set_value(demo->progress, demo->progress_value);

	if (ui_tab_is_selected(demo->tbars)) {
		ui_lock(demo->ui);
		ui_progress_paint(demo->progress);
		ui_unlock(demo->ui);
	}

	fibril_timer_set(demo->timer, 1000 * scrollbar_update_interval_ms,
	    ui_demo_timer_fun, (void *)demo);
}

/** Create UI demo menus.
 *
 * @param demo UI demo
 * @param fixed Fixed layout where menu bar is to be added
 * @param geom UI demo geometry
 * @return EOK on success or an error code
 */
static errno_t ui_demo_menus_create(ui_demo_t *demo, ui_fixed_t *fixed,
    ui_demo_geom_t *geom)
{
	ui_menu_bar_t *menubar = NULL;
	ui_menu_entry_t *mmsg;
	ui_menu_entry_t *mload;
	ui_menu_entry_t *mfoo;
	ui_menu_entry_t *mbar;
	ui_menu_entry_t *mfoobar;
	ui_menu_entry_t *msep;
	ui_menu_entry_t *mexit;
	ui_menu_entry_t *mmodify;
	ui_menu_entry_t *minsert_char;
	ui_menu_entry_t *mabout;
	errno_t rc;

	rc = ui_menu_bar_create(demo->ui, demo->window, &menubar);
	if (rc != EOK) {
		printf("Error creating menu bar.\n");
		goto error;
	}

	rc = ui_menu_dd_create(menubar, "~F~ile", NULL, &demo->mfile);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		goto error;
	}

	rc = ui_menu_entry_create(demo->mfile, "~M~essage", "", &mmsg);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		goto error;
	}

	ui_menu_entry_set_cb(mmsg, uidemo_file_message, (void *)demo);

	rc = ui_menu_entry_create(demo->mfile, "~C~onfirmation", "", &mmsg);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		goto error;
	}

	ui_menu_entry_set_cb(mmsg, uidemo_file_confirmation, (void *)demo);

	rc = ui_menu_entry_create(demo->mfile, "~L~oad", "", &mload);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		goto error;
	}

	ui_menu_entry_set_cb(mload, uidemo_file_load, (void *)demo);

	rc = ui_menu_entry_create(demo->mfile, "~F~oo", "Ctrl-Alt-Del", &mfoo);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		goto error;
	}

	rc = ui_menu_entry_create(demo->mfile, "~B~ar", "", &mbar);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		goto error;
	}

	rc = ui_menu_entry_create(demo->mfile, "F~o~obar", "", &mfoobar);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		goto error;
	}

	ui_menu_entry_set_disabled(mfoobar, true);

	rc = ui_menu_entry_sep_create(demo->mfile, &msep);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		goto error;
	}

	rc = ui_menu_entry_create(demo->mfile, "E~x~it", "Alt-F4", &mexit);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		goto error;
	}

	ui_menu_entry_set_cb(mexit, uidemo_file_exit, (void *)demo);

	rc = ui_menu_dd_create(menubar, "~E~dit", NULL, &demo->medit);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		goto error;
	}

	rc = ui_menu_entry_create(demo->medit, "~M~odify", "", &mmodify);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		goto error;
	}

	ui_menu_entry_set_cb(mmodify, uidemo_edit_modify, (void *)demo);

	rc = ui_menu_entry_create(demo->medit, "~I~nsert Character",
	    "", &minsert_char);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		goto error;
	}

	ui_menu_entry_set_cb(minsert_char, uidemo_edit_insert_character,
	    (void *)demo);

	rc = ui_menu_dd_create(menubar, "~P~references", NULL,
	    &demo->mpreferences);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		goto error;
	}

	rc = ui_menu_dd_create(menubar, "~H~elp", NULL, &demo->mhelp);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		goto error;
	}

	rc = ui_menu_entry_create(demo->mhelp, "~A~bout", "Ctrl-H, F1", &mabout);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		goto error;
	}

	ui_menu_bar_set_rect(menubar, &geom->mbar_rect);

	rc = ui_fixed_add(fixed, ui_menu_bar_ctl(menubar));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	demo->mbar = menubar;
	return EOK;
error:
	ui_menu_bar_destroy(menubar);
	return rc;
}

/** Create bitmap with moire patern for the image control.
 *
 * @param demo UI demo
 * @parma geom UI demo geometry
 * @param rect Place to store bitmap rectangle
 * @param rbitmap Place to store pointer to new bitmap.
 * @return EOK on success or an error code.
 */
static errno_t ui_demo_create_bitmap(ui_demo_t *demo, ui_demo_geom_t *geom,
    gfx_rect_t *rect, gfx_bitmap_t **rbitmap)
{
	gfx_bitmap_params_t bparams;
	gfx_bitmap_t *bitmap = NULL;
	gfx_context_t *gc;
	errno_t rc;

	gc = ui_window_get_gc(demo->window);

	gfx_bitmap_params_init(&bparams);
	gfx_rect_rtranslate(&geom->image_rect.p0, &geom->image_rect,
	    &bparams.rect);
	if (!ui_is_textmode(demo->ui)) {
		/* Adjust bitmap size for image frame. */
		bparams.rect.p1.x -= 2;
		bparams.rect.p1.y -= 2;
	}

	rc = gfx_bitmap_create(gc, &bparams, NULL, &bitmap);
	if (rc != EOK)
		goto error;

	rc = bitmap_moire(bitmap, bparams.rect.p1.x, bparams.rect.p1.y);
	if (rc != EOK)
		goto error;

	*rect = bparams.rect;
	*rbitmap = bitmap;
	return EOK;
error:
	if (bitmap != NULL)
		gfx_bitmap_destroy(bitmap);
	return rc;
}

/** Create UI demo basic tab.
 *
 * @param demo UI demo
 * @param tabset Tab set
 * @param geom UI demo geometry
 * @return EOK on success or an error code
 */
static errno_t ui_demo_tab_basic_create(ui_demo_t *demo, ui_tab_set_t *tabset,
    ui_demo_geom_t *geom)
{
	ui_fixed_t *bfixed = NULL;
	ui_entry_t *entry = NULL;
	ui_label_t *label = NULL;
	ui_pbutton_t *pb1 = NULL;
	ui_pbutton_t *pb2 = NULL;
	ui_image_t *image = NULL;
	ui_checkbox_t *checkbox = NULL;
	ui_rbutton_group_t *rbgroup = NULL;
	ui_rbutton_t *rbleft = NULL;
	ui_rbutton_t *rbcenter = NULL;
	ui_rbutton_t *rbright = NULL;
	ui_slider_t *slider = NULL;
	ui_scrollbar_t *hscrollbar = NULL;
	ui_scrollbar_t *vscrollbar = NULL;
	gfx_rect_t rect;
	gfx_bitmap_params_t bparams;
	gfx_bitmap_t *bitmap = NULL;
	gfx_coord2_t off;
	ui_resource_t *ui_res;
	errno_t rc;

	ui_res = ui_window_get_res(demo->window);

	rc = ui_tab_create(tabset, "Basic", &demo->tbasic);
	if (rc != EOK) {
		printf("Error creating tab.\n");
		goto error;
	}

	rc = ui_fixed_create(&bfixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		goto error;
	}

	rc = ui_entry_create(demo->window, "", &entry);
	if (rc != EOK) {
		printf("Error creating entry.\n");
		goto error;
	}

	ui_entry_set_rect(entry, &geom->entry_rect);
	ui_entry_set_halign(entry, gfx_halign_center);

	rc = ui_fixed_add(bfixed, ui_entry_ctl(entry));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	demo->entry = entry;
	entry = NULL;

	rc = ui_label_create(ui_res, "Text label", &label);
	if (rc != EOK) {
		printf("Error creating label.\n");
		goto error;
	}

	ui_label_set_rect(label, &geom->label_rect);
	ui_label_set_halign(label, gfx_halign_center);

	rc = ui_fixed_add(bfixed, ui_label_ctl(label));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	demo->label = label;
	label = NULL;

	rc = ui_pbutton_create(ui_res, "OK", &pb1);
	if (rc != EOK) {
		printf("Error creating button.\n");
		goto error;
	}

	ui_pbutton_set_cb(pb1, &pbutton_cb, (void *)demo);
	ui_pbutton_set_rect(pb1, &geom->pb1_rect);
	ui_pbutton_set_default(pb1, true);

	rc = ui_fixed_add(bfixed, ui_pbutton_ctl(pb1));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	demo->pb1 = pb1;
	pb1 = NULL;

	rc = ui_pbutton_create(ui_res, "Cancel", &pb2);
	if (rc != EOK) {
		printf("Error creating button.\n");
		goto error;
	}

	ui_pbutton_set_cb(pb2, &pbutton_cb, (void *)demo);
	ui_pbutton_set_rect(pb2, &geom->pb2_rect);

	rc = ui_fixed_add(bfixed, ui_pbutton_ctl(pb2));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	demo->pb2 = pb2;
	pb2 = NULL;

	rc = ui_demo_create_bitmap(demo, geom, &rect, &bitmap);
	if (rc != EOK)
		goto error;

	rc = ui_image_create(ui_res, bitmap, &rect, &image);
	if (rc != EOK) {
		printf("Error creating label.\n");
		goto error;
	}

	demo->bitmap = bitmap;
	bitmap = NULL;

	gfx_rect_translate(&off, &bparams.rect, &rect);

	/* Adjust for frame width (2 x 1 pixel) */
	if (!ui_is_textmode(demo->ui)) {
		ui_image_set_flags(image, ui_imgf_frame);
		rect.p1.x += 2;
		rect.p1.y += 2;
	}

	ui_image_set_rect(image, &geom->image_rect);

	rc = ui_fixed_add(bfixed, ui_image_ctl(image));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	demo->image = image;
	image = NULL;

	rc = ui_checkbox_create(ui_res, "Read only", &checkbox);
	if (rc != EOK) {
		printf("Error creating check box.\n");
		goto error;
	}

	ui_checkbox_set_cb(checkbox, &checkbox_cb, (void *)demo);

	ui_checkbox_set_rect(checkbox, &geom->checkbox_rect);

	rc = ui_fixed_add(bfixed, ui_checkbox_ctl(checkbox));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	demo->checkbox = checkbox;
	checkbox = NULL;

	rc = ui_rbutton_group_create(ui_res, &rbgroup);
	if (rc != EOK) {
		printf("Error creating radio button group.\n");
		goto error;
	}

	rc = ui_rbutton_create(rbgroup, "Left", (void *) &uidemo_halign[0],
	    &rbleft);
	if (rc != EOK) {
		printf("Error creating radio button.\n");
		goto error;
	}

	ui_rbutton_group_set_cb(rbgroup, &rbutton_group_cb,
	    (void *)demo);

	ui_rbutton_set_rect(rbleft, &geom->rbleft_rect);

	rc = ui_fixed_add(bfixed, ui_rbutton_ctl(rbleft));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	demo->rbleft = rbleft;
	rbleft = NULL;

	rc = ui_rbutton_create(rbgroup, "Center", (void *) &uidemo_halign[1],
	    &rbcenter);
	if (rc != EOK) {
		printf("Error creating radio button.\n");
		goto error;
	}

	ui_rbutton_set_rect(rbcenter, &geom->rbcenter_rect);
	ui_rbutton_select(rbcenter);

	rc = ui_fixed_add(bfixed, ui_rbutton_ctl(rbcenter));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	demo->rbcenter = rbcenter;
	rbcenter = NULL;

	rc = ui_rbutton_create(rbgroup, "Right", (void *) &uidemo_halign[2],
	    &rbright);
	if (rc != EOK) {
		printf("Error creating radio button.\n");
		goto error;
	}

	ui_rbutton_set_rect(rbright, &geom->rbright_rect);

	rc = ui_fixed_add(bfixed, ui_rbutton_ctl(rbright));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	demo->rbright = rbright;
	rbright = NULL;

	rc = ui_slider_create(ui_res, &slider);
	if (rc != EOK) {
		printf("Error creating button.\n");
		goto error;
	}

	ui_slider_set_cb(slider, &slider_cb, (void *)demo);
	ui_slider_set_rect(slider, &geom->slider_rect);

	rc = ui_fixed_add(bfixed, ui_slider_ctl(slider));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	demo->slider = slider;
	slider = NULL;

	rc = ui_scrollbar_create(demo->ui, demo->window, ui_sbd_horiz,
	    &hscrollbar);
	if (rc != EOK) {
		printf("Error creating scrollbar.\n");
		goto error;
	}

	ui_scrollbar_set_cb(hscrollbar, &scrollbar_cb, (void *)demo);
	ui_scrollbar_set_rect(hscrollbar, &geom->hscrollbar_rect);

	ui_scrollbar_set_thumb_length(hscrollbar,
	    ui_scrollbar_trough_length(hscrollbar) / 4);

	rc = ui_fixed_add(bfixed, ui_scrollbar_ctl(hscrollbar));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	demo->hscrollbar = hscrollbar;
	hscrollbar = NULL;

	rc = ui_scrollbar_create(demo->ui, demo->window, ui_sbd_vert,
	    &vscrollbar);
	if (rc != EOK) {
		printf("Error creating button.\n");
		goto error;
	}

	ui_scrollbar_set_cb(vscrollbar, &scrollbar_cb, (void *)demo);
	ui_scrollbar_set_rect(vscrollbar, &geom->vscrollbar_rect);

	ui_scrollbar_set_thumb_length(vscrollbar,
	    ui_scrollbar_trough_length(vscrollbar) / 4);

	rc = ui_fixed_add(bfixed, ui_scrollbar_ctl(vscrollbar));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	demo->vscrollbar = vscrollbar;

	ui_tab_add(demo->tbasic, ui_fixed_ctl(bfixed));
	return EOK;
error:
	ui_scrollbar_destroy(vscrollbar);
	ui_scrollbar_destroy(hscrollbar);
	ui_slider_destroy(slider);
	ui_rbutton_destroy(rbright);
	ui_rbutton_destroy(rbcenter);
	ui_rbutton_destroy(rbleft);
	ui_rbutton_group_destroy(rbgroup);
	ui_checkbox_destroy(checkbox);
	ui_image_destroy(image);
	if (bitmap != NULL)
		gfx_bitmap_destroy(bitmap);
	ui_pbutton_destroy(pb1);
	ui_pbutton_destroy(pb2);
	ui_label_destroy(label);
	ui_entry_destroy(entry);
	ui_fixed_destroy(bfixed);
	return rc;
}

/** Create UI demo lists tab.
 *
 * @param demo UI demo
 * @param tabset Tab set
 * @param geom UI demo geometry
 * @return EOK on success or an error code
 */
static errno_t ui_demo_tab_lists_create(ui_demo_t *demo, ui_tab_set_t *tabset,
    ui_demo_geom_t *geom)
{
	ui_fixed_t *lfixed = NULL;
	ui_list_t *list = NULL;
	ui_list_entry_attr_t eattr;
	errno_t rc;

	rc = ui_tab_create(tabset, "Lists", &demo->tlists);
	if (rc != EOK) {
		printf("Error creating tab.\n");
		goto error;
	}

	rc = ui_fixed_create(&lfixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		goto error;
	}

	rc = ui_list_create(demo->window, false, &list);
	if (rc != EOK) {
		printf("Error creating list.\n");
		goto error;
	}

	ui_list_entry_attr_init(&eattr);

	eattr.caption = "One";
	rc = ui_list_entry_append(list, &eattr, NULL);
	if (rc != EOK) {
		printf("Error adding list entry.\n");
		goto error;
	}

	eattr.caption = "Two";
	rc = ui_list_entry_append(list, &eattr, NULL);
	if (rc != EOK) {
		printf("Error adding list entry.\n");
		goto error;
	}

	eattr.caption = "Three";
	rc = ui_list_entry_append(list, &eattr, NULL);
	if (rc != EOK) {
		printf("Error adding list entry.\n");
		goto error;
	}

	eattr.caption = "Four";
	rc = ui_list_entry_append(list, &eattr, NULL);
	if (rc != EOK) {
		printf("Error adding list entry.\n");
		goto error;
	}

	eattr.caption = "Five";
	rc = ui_list_entry_append(list, &eattr, NULL);
	if (rc != EOK) {
		printf("Error adding list entry.\n");
		goto error;
	}

	eattr.caption = "Six";
	rc = ui_list_entry_append(list, &eattr, NULL);
	if (rc != EOK) {
		printf("Error adding list entry.\n");
		goto error;
	}

	ui_list_set_rect(list, &geom->list_rect);

	rc = ui_fixed_add(lfixed, ui_list_ctl(list));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	demo->list = list;
	list = NULL;

	ui_tab_add(demo->tlists, ui_fixed_ctl(lfixed));
	return EOK;
error:
	ui_list_destroy(list);
	ui_fixed_destroy(lfixed);
	return rc;
}

/** Create UI demo bars tab.
 *
 * @param demo UI demo
 * @param tabset Tab set
 * @param geom UI demo geometry
 * @return EOK on success or an error code
 */
static errno_t ui_demo_tab_bars_create(ui_demo_t *demo, ui_tab_set_t *tabset,
    ui_demo_geom_t *geom)
{
	ui_fixed_t *bars_fixed = NULL;
	ui_progress_t *progress = NULL;
	ui_resource_t *ui_res;
	errno_t rc;

	ui_res = ui_window_get_res(demo->window);

	rc = ui_tab_create(tabset, "Bars", &demo->tbars);
	if (rc != EOK) {
		printf("Error creating tab.\n");
		goto error;
	}

	rc = ui_fixed_create(&bars_fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		goto error;
	}

	rc = ui_progress_create(ui_res, 0, &progress);
	if (rc != EOK) {
		printf("Error creating entry.\n");
		goto error;
	}

	ui_progress_set_rect(progress, &geom->progress_rect);

	rc = ui_fixed_add(bars_fixed, ui_progress_ctl(progress));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	demo->progress = progress;
	progress = NULL;

	ui_tab_add(demo->tbars, ui_fixed_ctl(bars_fixed));
	bars_fixed = NULL;

	return EOK;
error:
	ui_progress_destroy(progress);
	ui_fixed_destroy(bars_fixed);
	return rc;
}

/** Get UI demo geometry.
 *
 * @param demo UI demo
 * @param arect Application rectangle
 * @param geom Place to store computed geometry.
 */
static void ui_demo_get_geom(ui_demo_t *demo, gfx_rect_t *arect,
    ui_demo_geom_t *geom)
{
	gfx_coord_t center;

	center = (arect->p0.x + arect->p1.x) / 2;

	/* FIXME: Auto layout */
	if (ui_is_textmode(demo->ui)) {
		geom->mbar_rect.p0.x = arect->p0.x;
		geom->mbar_rect.p0.y = arect->p0.y;
		geom->mbar_rect.p1.x = arect->p1.x - 2;
		geom->mbar_rect.p1.y = arect->p0.x + 1;
	} else {
		geom->mbar_rect.p0.x = arect->p0.x + 4;
		geom->mbar_rect.p0.y = arect->p0.y + 4;
		geom->mbar_rect.p1.x = arect->p1.x - 4;
		geom->mbar_rect.p1.y = arect->p0.y + 26;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(demo->ui)) {
		geom->tabset_rect.p0.x = arect->p0.x + 1;
		geom->tabset_rect.p0.y = arect->p0.y + 1;
		geom->tabset_rect.p1.x = arect->p1.x - 1;
		geom->tabset_rect.p1.y = arect->p1.y;
	} else {
		geom->tabset_rect.p0.x = arect->p0.x;
		geom->tabset_rect.p0.y = arect->p0.y + 27;
		geom->tabset_rect.p1.x = arect->p1.x - 1;
		geom->tabset_rect.p1.y = arect->p1.y - 1;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(demo->ui)) {
		geom->entry_rect.p0.x = arect->p0.x + 3;
		geom->entry_rect.p0.y = arect->p0.y + 4;
		geom->entry_rect.p1.x = arect->p1.x - 4;
		geom->entry_rect.p1.y = arect->p0.y + 5;
	} else {
		geom->entry_rect.p0.x = arect->p0.x + 11;
		geom->entry_rect.p0.y = arect->p0.y + 62;
		geom->entry_rect.p1.x = arect->p1.x - 46;
		geom->entry_rect.p1.y = arect->p0.y + 87;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(demo->ui)) {
		geom->label_rect.p0.x = arect->p0.x + 3;
		geom->label_rect.p0.y = arect->p0.y + 6;
		geom->label_rect.p1.x = arect->p1.x - 4;
		geom->label_rect.p1.y = arect->p0.y + 7;
	} else {
		geom->label_rect.p0.x = arect->p0.x + 56;
		geom->label_rect.p0.y = arect->p0.y + 97;
		geom->label_rect.p1.x = arect->p1.x - 91;
		geom->label_rect.p1.y = arect->p0.y + 110;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(demo->ui)) {
		geom->pb1_rect.p0.x = center - 19;
		geom->pb1_rect.p0.y = arect->p0.y + 8;
		geom->pb1_rect.p1.x = center - 8;
		geom->pb1_rect.p1.y = arect->p0.y + 9;
	} else {
		geom->pb1_rect.p0.x = center - 110;
		geom->pb1_rect.p0.y = arect->p0.y + 120;
		geom->pb1_rect.p1.x = center - 22;
		geom->pb1_rect.p1.y = arect->p0.y + 148;
	}

	if (ui_is_textmode(demo->ui)) {
		geom->pb2_rect.p0.x = center + 7;
		geom->pb2_rect.p0.y = arect->p0.y + 8;
		geom->pb2_rect.p1.x = center + 18;
		geom->pb2_rect.p1.y = arect->p0.y + 9;
	} else {
		geom->pb2_rect.p0.x = center - 12;
		geom->pb2_rect.p0.y = arect->p0.y + 120;
		geom->pb2_rect.p1.x = center + 78;
		geom->pb2_rect.p1.y = arect->p0.y + 148;
	}

	if (ui_is_textmode(demo->ui)) {
		geom->image_rect.p0.x = arect->p0.x + 3;
		geom->image_rect.p0.y = arect->p0.y + 10;
		geom->image_rect.p1.x = arect->p1.x - 4;
		geom->image_rect.p1.y = arect->p1.y - 11;
	} else {
		geom->image_rect.p0.x = arect->p0.x + 11;
		geom->image_rect.p0.y = arect->p0.y + 166;
		geom->image_rect.p1.x = arect->p1.x - 48;
		geom->image_rect.p1.y = arect->p1.y - 190;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(demo->ui)) {
		geom->checkbox_rect.p0.x = arect->p0.x + 3;
		geom->checkbox_rect.p0.y = arect->p1.y - 10;
		geom->checkbox_rect.p1.x = arect->p0.x + 13;
		geom->checkbox_rect.p1.y = arect->p1.y - 9;
	} else {
		geom->checkbox_rect.p0.x = arect->p0.x + 11;
		geom->checkbox_rect.p0.y = arect->p1.y - 181;
		geom->checkbox_rect.p1.x = arect->p0.x + 136;
		geom->checkbox_rect.p1.y = arect->p1.y - 161;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(demo->ui)) {
		geom->rbleft_rect.p0.x = arect->p0.x + 3;
		geom->rbleft_rect.p0.y = arect->p1.y - 7;
		geom->rbleft_rect.p1.x = arect->p0.x + 13;
		geom->rbleft_rect.p1.y = arect->p1.y - 6;
	} else {
		geom->rbleft_rect.p0.x = arect->p0.x + 11;
		geom->rbleft_rect.p0.y = arect->p1.y - 151;
		geom->rbleft_rect.p1.x = arect->p0.x + 136;
		geom->rbleft_rect.p1.y = arect->p1.y - 131;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(demo->ui)) {
		geom->rbcenter_rect.p0.x = arect->p0.x + 3;
		geom->rbcenter_rect.p0.y = arect->p1.y - 6;
		geom->rbcenter_rect.p1.x = arect->p0.x + 13;
		geom->rbcenter_rect.p1.y = arect->p1.y - 5;
	} else {
		geom->rbcenter_rect.p0.x = arect->p0.x + 11;
		geom->rbcenter_rect.p0.y = arect->p1.y - 121;
		geom->rbcenter_rect.p1.x = arect->p0.x + 136;
		geom->rbcenter_rect.p1.y = arect->p1.y - 101;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(demo->ui)) {
		geom->rbright_rect.p0.x = arect->p0.x + 3;
		geom->rbright_rect.p0.y = arect->p1.y - 5;
		geom->rbright_rect.p1.x = arect->p0.x + 13;
		geom->rbright_rect.p1.y = arect->p1.y - 4;
	} else {
		geom->rbright_rect.p0.x = arect->p0.x + 11;
		geom->rbright_rect.p0.y = arect->p1.y - 91;
		geom->rbright_rect.p1.x = arect->p0.x + 136;
		geom->rbright_rect.p1.y = arect->p1.y - 71;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(demo->ui)) {
		geom->slider_rect.p0.x = arect->p0.x + 3;
		geom->slider_rect.p0.y = arect->p1.y - 3;
		geom->slider_rect.p1.x = arect->p0.x + 31;
		geom->slider_rect.p1.y = arect->p1.y - 2;
	} else {
		geom->slider_rect.p0.x = arect->p0.x + 11;
		geom->slider_rect.p0.y = arect->p1.y - 61;
		geom->slider_rect.p1.x = arect->p0.x + 126;
		geom->slider_rect.p1.y = arect->p1.y - 41;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(demo->ui)) {
		geom->hscrollbar_rect.p0.x = arect->p0.x + 3;
		geom->hscrollbar_rect.p0.y = arect->p1.y - 2;
		geom->hscrollbar_rect.p1.x = arect->p1.x - 3;
		geom->hscrollbar_rect.p1.y = arect->p1.y - 1;
	} else {
		geom->hscrollbar_rect.p0.x = arect->p0.x + 11;
		geom->hscrollbar_rect.p0.y = arect->p1.y - 31;
		geom->hscrollbar_rect.p1.x = arect->p1.x - 31;
		geom->hscrollbar_rect.p1.y = arect->p1.y - 8;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(demo->ui)) {
		geom->vscrollbar_rect.p0.x = arect->p1.x - 3;
		geom->vscrollbar_rect.p0.y = arect->p0.y + 4;
		geom->vscrollbar_rect.p1.x = arect->p1.x - 2;
		geom->vscrollbar_rect.p1.y = arect->p1.y - 2;
	} else {
		geom->vscrollbar_rect.p0.x = arect->p1.x - 31;
		geom->vscrollbar_rect.p0.y = arect->p0.y + 62;
		geom->vscrollbar_rect.p1.x = arect->p1.x - 8;
		geom->vscrollbar_rect.p1.y = arect->p1.y - 31;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(demo->ui)) {
		geom->list_rect.p0.x = arect->p0.x + 3;
		geom->list_rect.p0.y = arect->p0.y + 4;
		geom->list_rect.p1.x = arect->p1.x - 4;
		geom->list_rect.p1.y = arect->p0.y + 9;
	} else {
		geom->list_rect.p0.x = arect->p0.x + 11;
		geom->list_rect.p0.y = arect->p0.y + 62;
		geom->list_rect.p1.x = arect->p1.x - 6;
		geom->list_rect.p1.y = arect->p0.x + 147;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(demo->ui)) {
		geom->progress_rect.p0.x = arect->p0.x + 3;
		geom->progress_rect.p0.y = arect->p0.y + 4;
		geom->progress_rect.p1.x = arect->p1.x - 3;
		geom->progress_rect.p1.y = arect->p0.y + 5;
	} else {
		geom->progress_rect.p0.x = arect->p0.x + 15;
		geom->progress_rect.p0.y = arect->p0.y + 62;
		geom->progress_rect.p1.x = arect->p1.x - 8;
		geom->progress_rect.p1.y = arect->p0.y + 87;
	}
}

/** Create UI demo.
 *
 * @param display_spec Display specification
 * @param rdemo Place to store pointer to new demo
 * @return EOK on success or an error code
 */
static errno_t ui_demo_create(const char *display_spec, ui_demo_t **rdemo)
{
	ui_wnd_params_t params;
	ui_demo_t *demo = NULL;
	ui_fixed_t *fixed = NULL;
	ui_fixed_t *bars_fixed = NULL;
	ui_progress_t *progress = NULL;
	ui_resource_t *ui_res;
	ui_tab_set_t *tabset = NULL;
	gfx_rect_t arect;
	ui_demo_geom_t geom;
	errno_t rc;

	demo = calloc(1, sizeof(ui_demo_t));
	if (demo == NULL) {
		rc = ENOMEM;
		goto error;
	}

	rc = ui_create(display_spec, &demo->ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n", display_spec);
		goto error;
	}

	ui_wnd_params_init(&params);
	params.caption = "UI Demo";
	params.style |= ui_wds_maximize_btn | ui_wds_resizable;

	/* FIXME: Auto layout */
	if (ui_is_textmode(demo->ui)) {
		params.rect.p0.x = 0;
		params.rect.p0.y = 0;
		params.rect.p1.x = 46;
		params.rect.p1.y = 25;
	} else {
		params.rect.p0.x = 0;
		params.rect.p0.y = 0;
		params.rect.p1.x = 255;
		params.rect.p1.y = 410;
	}

	/* Only allow making the window larger */
	gfx_rect_dims(&params.rect, &params.min_size);

	rc = ui_window_create(demo->ui, &params, &demo->window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		goto error;
	}

	ui_window_set_cb(demo->window, &window_cb, (void *)demo);

	ui_res = ui_window_get_res(demo->window);

	rc = ui_fixed_create(&fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		goto error;
	}

	ui_window_get_app_rect(demo->window, &arect);
	ui_demo_get_geom(demo, &arect, &geom);

	rc = ui_demo_menus_create(demo, fixed, &geom);
	if (rc != EOK)
		goto error;

	rc = ui_tab_set_create(ui_res, &tabset);
	if (rc != EOK) {
		printf("Error creating tab set.\n");
		goto error;
	}

	ui_tab_set_set_rect(tabset, &geom.tabset_rect);

	rc = ui_demo_tab_basic_create(demo, tabset, &geom);
	if (rc != EOK)
		goto error;

	rc = ui_demo_tab_lists_create(demo, tabset, &geom);
	if (rc != EOK)
		goto error;

	rc = ui_demo_tab_bars_create(demo, tabset, &geom);
	if (rc != EOK)
		goto error;

	rc = ui_fixed_add(fixed, ui_tab_set_ctl(tabset));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	demo->tabset = tabset;
	tabset = NULL;

	ui_window_add(demo->window, ui_fixed_ctl(fixed));
	fixed = NULL;

	rc = ui_window_paint(demo->window);
	if (rc != EOK)
		goto error;

	demo->timer = fibril_timer_create(NULL);
	if (demo->timer == NULL) {
		printf("Error creating timer.\n");
		return ENOMEM;
	}

	fibril_timer_set(demo->timer, 1000 * scrollbar_update_interval_ms,
	    ui_demo_timer_fun, (void *)demo);

	*rdemo = demo;
	return EOK;
error:
	ui_progress_destroy(progress);
	ui_tab_set_destroy(tabset);
	ui_fixed_destroy(bars_fixed);
	ui_demo_destroy(demo);
	return rc;
}

/** Destroy UI demo.
 *
 * @param demo UI demo or @c NULL
 */
static void ui_demo_destroy(ui_demo_t *demo)
{
	if (demo == NULL)
		return;

	if (demo->timer != NULL) {
		fibril_timer_clear(demo->timer);
		fibril_timer_destroy(demo->timer);
	}

	ui_rbutton_group_destroy(demo->rbgroup);
	ui_window_destroy(demo->window);
	ui_destroy(demo->ui);

	free(demo);
}

/** Run UI demo on display server.
 *
 * @param display_spec Display specification
 * @return EOK on success or an error code
 */
static errno_t ui_demo(const char *display_spec)
{
	ui_demo_t *demo;
	errno_t rc;

	rc = ui_demo_create(display_spec, &demo);
	if (rc != EOK)
		return rc;

	ui_run(demo->ui);

	ui_demo_destroy(demo);
	return EOK;
}

/** Fill bitmap with moire pattern.
 *
 * @param bitmap Bitmap
 * @param w Bitmap width
 * @param h Bitmap height
 * @return EOK on success or an error code
 */
static errno_t bitmap_moire(gfx_bitmap_t *bitmap, gfx_coord_t w, gfx_coord_t h)
{
	int i, j;
	int k;
	pixelmap_t pixelmap;
	gfx_bitmap_alloc_t alloc;
	errno_t rc;

	rc = gfx_bitmap_get_alloc(bitmap, &alloc);
	if (rc != EOK)
		return rc;

	/* In absence of anything else, use pixelmap */
	pixelmap.width = w;
	pixelmap.height = h;
	pixelmap.data = alloc.pixels;

	for (i = 0; i < w; i++) {
		for (j = 0; j < h; j++) {
			k = i * i + j * j;
			pixelmap_put_pixel(&pixelmap, i, j,
			    PIXEL(0, k, k, 255 - k));
		}
	}

	return EOK;
}

static void print_syntax(void)
{
	printf("Syntax: uidemo [-d <display-spec>]\n");
}

int main(int argc, char *argv[])
{
	const char *display_spec = UI_ANY_DEFAULT;
	errno_t rc;
	int i;

	i = 1;
	while (i < argc && argv[i][0] == '-') {
		if (str_cmp(argv[i], "-d") == 0) {
			++i;
			if (i >= argc) {
				printf("Argument missing.\n");
				print_syntax();
				return 1;
			}

			display_spec = argv[i++];
		} else {
			printf("Invalid option '%s'.\n", argv[i]);
			print_syntax();
			return 1;
		}
	}

	if (i < argc) {
		print_syntax();
		return 1;
	}

	rc = ui_demo(display_spec);
	if (rc != EOK)
		return 1;

	return 0;
}

/** @}
 */
