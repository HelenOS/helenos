/*
 * Copyright (c) 2021 Jiri Svoboda
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

#include <gfx/bitmap.h>
#include <gfx/coord.h>
#include <io/pixelmap.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <ui/entry.h>
#include <ui/filedialog.h>
#include <ui/fixed.h>
#include <ui/image.h>
#include <ui/label.h>
#include <ui/menubar.h>
#include <ui/menuentry.h>
#include <ui/menu.h>
#include <ui/msgdialog.h>
#include <ui/pbutton.h>
#include <ui/promptdialog.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include "uidemo.h"

static errno_t bitmap_moire(gfx_bitmap_t *, gfx_coord_t, gfx_coord_t);

static void wnd_close(ui_window_t *, void *);

static ui_window_cb_t window_cb = {
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

static void uidemo_file_load(ui_menu_entry_t *, void *);
static void uidemo_file_message(ui_menu_entry_t *, void *);
static void uidemo_file_exit(ui_menu_entry_t *, void *);
static void uidemo_edit_modify(ui_menu_entry_t *, void *);

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

static void msg_dialog_button(ui_msg_dialog_t *, void *, unsigned);
static void msg_dialog_close(ui_msg_dialog_t *, void *);

static ui_msg_dialog_cb_t msg_dialog_cb = {
	.button = msg_dialog_button,
	.close = msg_dialog_close
};

/** Horizontal alignment selected by each radio button */
static const gfx_halign_t uidemo_halign[3] = {
	gfx_halign_left,
	gfx_halign_center,
	gfx_halign_right
};

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

/** Display a message window.
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

	ui_msg_dialog_set_cb(dialog, &msg_dialog_cb, &demo);
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

	ui_msg_dialog_set_cb(dialog, &msg_dialog_cb, &demo);
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
		printf("Error creating message dialog.\n");
		return;
	}

	ui_prompt_dialog_set_cb(dialog, &prompt_dialog_cb, demo);
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
 * @param dialog File dialog
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
 * @param dialog File dialog
 * @param arg Argument (ui_demo_t *)
 */
static void prompt_dialog_close(ui_prompt_dialog_t *dialog, void *arg)
{
	ui_demo_t *demo = (ui_demo_t *) arg;

	(void) demo;
	ui_prompt_dialog_destroy(dialog);
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

/** Run UI demo on display server. */
static errno_t ui_demo(const char *display_spec)
{
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	ui_demo_t demo;
	gfx_rect_t rect;
	gfx_context_t *gc;
	ui_resource_t *ui_res;
	gfx_bitmap_params_t bparams;
	gfx_bitmap_t *bitmap;
	gfx_coord2_t off;
	ui_menu_entry_t *mmsg;
	ui_menu_entry_t *mload;
	ui_menu_entry_t *mfoo;
	ui_menu_entry_t *mbar;
	ui_menu_entry_t *mfoobar;
	ui_menu_entry_t *msep;
	ui_menu_entry_t *mexit;
	ui_menu_entry_t *mmodify;
	ui_menu_entry_t *mabout;
	errno_t rc;

	rc = ui_create(display_spec, &ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n", display_spec);
		return rc;
	}

	memset((void *) &demo, 0, sizeof(demo));
	demo.ui = ui;

	ui_wnd_params_init(&params);
	params.caption = "UI Demo";
	params.style |= ui_wds_resizable;

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		params.rect.p0.x = 0;
		params.rect.p0.y = 0;
		params.rect.p1.x = 44;
		params.rect.p1.y = 21;
	} else {
		params.rect.p0.x = 0;
		params.rect.p0.y = 0;
		params.rect.p1.x = 220;
		params.rect.p1.y = 350;
	}

	rc = ui_window_create(ui, &params, &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		return rc;
	}

	ui_window_set_cb(window, &window_cb, (void *) &demo);
	demo.window = window;

	ui_res = ui_window_get_res(window);
	gc = ui_window_get_gc(window);

	rc = ui_fixed_create(&demo.fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		return rc;
	}

	rc = ui_menu_bar_create(ui, window, &demo.mbar);
	if (rc != EOK) {
		printf("Error creating menu bar.\n");
		return rc;
	}

	rc = ui_menu_create(demo.mbar, "File", &demo.mfile);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	rc = ui_menu_entry_create(demo.mfile, "Message", "", &mmsg);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(mmsg, uidemo_file_message, (void *) &demo);

	rc = ui_menu_entry_create(demo.mfile, "Load", "", &mload);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(mload, uidemo_file_load, (void *) &demo);

	rc = ui_menu_entry_create(demo.mfile, "Foo", "Ctrl-Alt-Del", &mfoo);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	rc = ui_menu_entry_create(demo.mfile, "Bar", "", &mbar);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	rc = ui_menu_entry_create(demo.mfile, "Foobar", "", &mfoobar);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	rc = ui_menu_entry_sep_create(demo.mfile, &msep);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	rc = ui_menu_entry_create(demo.mfile, "Exit", "Alt-F4", &mexit);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(mexit, uidemo_file_exit, (void *) &demo);

	rc = ui_menu_create(demo.mbar, "Edit", &demo.medit);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	rc = ui_menu_entry_create(demo.medit, "Modify", "", &mmodify);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	ui_menu_entry_set_cb(mmodify, uidemo_edit_modify, (void *) &demo);

	rc = ui_menu_create(demo.mbar, "Preferences", &demo.mpreferences);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	rc = ui_menu_create(demo.mbar, "Help", &demo.mhelp);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	rc = ui_menu_entry_create(demo.mhelp, "About", "Ctrl-H, F1", &mabout);
	if (rc != EOK) {
		printf("Error creating menu.\n");
		return rc;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 1;
		rect.p0.y = 1;
		rect.p1.x = 43;
		rect.p1.y = 2;
	} else {
		rect.p0.x = 4;
		rect.p0.y = 30;
		rect.p1.x = 216;
		rect.p1.y = 52;
	}
	ui_menu_bar_set_rect(demo.mbar, &rect);

	rc = ui_fixed_add(demo.fixed, ui_menu_bar_ctl(demo.mbar));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	rc = ui_entry_create(window, "", &demo.entry);
	if (rc != EOK) {
		printf("Error creating entry.\n");
		return rc;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 2;
		rect.p0.y = 3;
		rect.p1.x = 42;
		rect.p1.y = 4;
	} else {
		rect.p0.x = 15;
		rect.p0.y = 53;
		rect.p1.x = 205;
		rect.p1.y = 78;
	}

	ui_entry_set_rect(demo.entry, &rect);
	ui_entry_set_halign(demo.entry, gfx_halign_center);

	rc = ui_fixed_add(demo.fixed, ui_entry_ctl(demo.entry));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	rc = ui_label_create(ui_res, "Text label", &demo.label);
	if (rc != EOK) {
		printf("Error creating label.\n");
		return rc;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 2;
		rect.p0.y = 5;
		rect.p1.x = 42;
		rect.p1.y = 6;
	} else {
		rect.p0.x = 60;
		rect.p0.y = 88;
		rect.p1.x = 160;
		rect.p1.y = 101;
	}

	ui_label_set_rect(demo.label, &rect);
	ui_label_set_halign(demo.label, gfx_halign_center);

	rc = ui_fixed_add(demo.fixed, ui_label_ctl(demo.label));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	rc = ui_pbutton_create(ui_res, "OK", &demo.pb1);
	if (rc != EOK) {
		printf("Error creating button.\n");
		return rc;
	}

	ui_pbutton_set_cb(demo.pb1, &pbutton_cb, (void *) &demo);

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 2;
		rect.p0.y = 7;
		rect.p1.x = 12;
		rect.p1.y = 8;
	} else {
		rect.p0.x = 15;
		rect.p0.y = 111;
		rect.p1.x = 105;
		rect.p1.y = 139;
	}

	ui_pbutton_set_rect(demo.pb1, &rect);

	ui_pbutton_set_default(demo.pb1, true);

	rc = ui_fixed_add(demo.fixed, ui_pbutton_ctl(demo.pb1));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	rc = ui_pbutton_create(ui_res, "Cancel", &demo.pb2);
	if (rc != EOK) {
		printf("Error creating button.\n");
		return rc;
	}

	ui_pbutton_set_cb(demo.pb2, &pbutton_cb, (void *) &demo);

	if (ui_is_textmode(ui)) {
		rect.p0.x = 32;
		rect.p0.y = 7;
		rect.p1.x = 42;
		rect.p1.y = 8;
	} else {
		rect.p0.x = 115;
		rect.p0.y = 111;
		rect.p1.x = 205;
		rect.p1.y = 139;
	}

	ui_pbutton_set_rect(demo.pb2, &rect);

	rc = ui_fixed_add(demo.fixed, ui_pbutton_ctl(demo.pb2));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	gfx_bitmap_params_init(&bparams);
	if (ui_is_textmode(ui)) {
		bparams.rect.p0.x = 0;
		bparams.rect.p0.y = 0;
		bparams.rect.p1.x = 40;
		bparams.rect.p1.y = 2;
	} else {
		bparams.rect.p0.x = 0;
		bparams.rect.p0.y = 0;
		bparams.rect.p1.x = 188;
		bparams.rect.p1.y = 24;
	}

	rc = gfx_bitmap_create(gc, &bparams, NULL, &bitmap);
	if (rc != EOK)
		return rc;

	rc = bitmap_moire(bitmap, bparams.rect.p1.x, bparams.rect.p1.y);
	if (rc != EOK)
		return rc;

	rc = ui_image_create(ui_res, bitmap, &params.rect, &demo.image);
	if (rc != EOK) {
		printf("Error creating label.\n");
		return rc;
	}

	if (ui_is_textmode(ui)) {
		off.x = 2;
		off.y = 9;
	} else {
		off.x = 15;
		off.y = 155;
	}

	gfx_rect_translate(&off, &bparams.rect, &rect);

	/* Adjust for frame width (2 x 1 pixel) */
	if (!ui_is_textmode(ui)) {
		ui_image_set_flags(demo.image, ui_imgf_frame);
		rect.p1.x += 2;
		rect.p1.y += 2;
	}

	ui_image_set_rect(demo.image, &rect);

	rc = ui_fixed_add(demo.fixed, ui_image_ctl(demo.image));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	rc = ui_checkbox_create(ui_res, "Read only", &demo.checkbox);
	if (rc != EOK) {
		printf("Error creating check box.\n");
		return rc;
	}

	ui_checkbox_set_cb(demo.checkbox, &checkbox_cb, (void *) &demo);

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 2;
		rect.p0.y = 12;
		rect.p1.x = 12;
		rect.p1.y = 13;
	} else {
		rect.p0.x = 15;
		rect.p0.y = 190;
		rect.p1.x = 140;
		rect.p1.y = 210;
	}

	ui_checkbox_set_rect(demo.checkbox, &rect);

	rc = ui_fixed_add(demo.fixed, ui_checkbox_ctl(demo.checkbox));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	rc = ui_rbutton_group_create(ui_res, &demo.rbgroup);
	if (rc != EOK) {
		printf("Error creating radio button group.\n");
		return rc;
	}

	rc = ui_rbutton_create(demo.rbgroup, "Left", (void *) &uidemo_halign[0],
	    &demo.rbleft);
	if (rc != EOK) {
		printf("Error creating radio button.\n");
		return rc;
	}

	ui_rbutton_group_set_cb(demo.rbgroup, &rbutton_group_cb,
	    (void *) &demo);

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 2;
		rect.p0.y = 14;
		rect.p1.x = 12;
		rect.p1.y = 15;
	} else {
		rect.p0.x = 15;
		rect.p0.y = 220;
		rect.p1.x = 140;
		rect.p1.y = 240;
	}
	ui_rbutton_set_rect(demo.rbleft, &rect);

	rc = ui_fixed_add(demo.fixed, ui_rbutton_ctl(demo.rbleft));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	rc = ui_rbutton_create(demo.rbgroup, "Center", (void *) &uidemo_halign[1],
	    &demo.rbcenter);
	if (rc != EOK) {
		printf("Error creating radio button.\n");
		return rc;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 2;
		rect.p0.y = 15;
		rect.p1.x = 12;
		rect.p1.y = 16;
	} else {
		rect.p0.x = 15;
		rect.p0.y = 250;
		rect.p1.x = 140;
		rect.p1.y = 270;
	}
	ui_rbutton_set_rect(demo.rbcenter, &rect);
	ui_rbutton_select(demo.rbcenter);

	rc = ui_fixed_add(demo.fixed, ui_rbutton_ctl(demo.rbcenter));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	rc = ui_rbutton_create(demo.rbgroup, "Right", (void *) &uidemo_halign[2],
	    &demo.rbright);
	if (rc != EOK) {
		printf("Error creating radio button.\n");
		return rc;
	}

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 2;
		rect.p0.y = 16;
		rect.p1.x = 12;
		rect.p1.y = 17;
	} else {
		rect.p0.x = 15;
		rect.p0.y = 280;
		rect.p1.x = 140;
		rect.p1.y = 300;
	}
	ui_rbutton_set_rect(demo.rbright, &rect);

	rc = ui_fixed_add(demo.fixed, ui_rbutton_ctl(demo.rbright));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	rc = ui_slider_create(ui_res, "Slide!", &demo.slider);
	if (rc != EOK) {
		printf("Error creating button.\n");
		return rc;
	}

	ui_slider_set_cb(demo.slider, &slider_cb, (void *) &demo);

	/* FIXME: Auto layout */
	if (ui_is_textmode(ui)) {
		rect.p0.x = 2;
		rect.p0.y = 18;
		rect.p1.x = 12;
		rect.p1.y = 19;
	} else {
		rect.p0.x = 15;
		rect.p0.y = 310;
		rect.p1.x = 130;
		rect.p1.y = 330;
	}

	ui_slider_set_rect(demo.slider, &rect);

	rc = ui_fixed_add(demo.fixed, ui_slider_ctl(demo.slider));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		return rc;
	}

	ui_window_add(window, ui_fixed_ctl(demo.fixed));

	rc = ui_window_paint(window);
	if (rc != EOK) {
		printf("Error painting window.\n");
		return rc;
	}

	ui_run(ui);

	ui_window_destroy(window);
	ui_destroy(ui);

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
	const char *display_spec = UI_DISPLAY_DEFAULT;
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
