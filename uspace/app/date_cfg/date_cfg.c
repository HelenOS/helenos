/*
 * Copyright (c) 2025 Wayne Michael Thornton (WMT) <wmthornton-dev@outlook.com>
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

/** @addtogroup date_cfg
 * @{
 */
/** @file Date configuration application (in UI)
 */

#include <gfx/coord.h>
#include <stdio.h>
#include <str.h>
#include <stdlib.h>
#include <ui/fixed.h>
#include <ui/label.h>
#include <ui/pbutton.h>
#include <ui/entry.h>
#include <ui/resource.h>
#include <ui/ui.h>
#include <ui/window.h>
#include <device/clock_dev.h>
#include <loc.h>
#include <time.h>
#include "date_cfg.h"

static void wnd_close(ui_window_t *, void *);
static void ok_clicked(ui_pbutton_t *, void *);
static void set_clicked(ui_pbutton_t *, void *);
static errno_t get_current_time(struct tm *);
static errno_t set_system_time(struct tm *);
static void update_time_display(date_cfg_t *);
static errno_t parse_date_time(date_cfg_t *);

static ui_window_cb_t window_cb = {
	.close = wnd_close
};

static ui_pbutton_cb_t button_cb = {
	.clicked = ok_clicked
};

static ui_pbutton_cb_t set_button_cb = {
	.clicked = set_clicked
};

/** Window close button was clicked.
 *
 * @param window Window
 * @param arg Argument (date_cfg)
 */
static void wnd_close(ui_window_t *window, void *arg)
{
	date_cfg_t *date_cfg = (date_cfg_t *) arg;
	ui_quit(date_cfg->ui);
}

/** OK button was clicked.
 *
 * @param button Button that was clicked
 * @param arg Argument (date_cfg)
 */
static void ok_clicked(ui_pbutton_t *button, void *arg)
{
	date_cfg_t *date_cfg = (date_cfg_t *) arg;
	ui_quit(date_cfg->ui);
}

/** Set button was clicked.
 *
 * @param button Button that was clicked
 * @param arg Argument (date_cfg)
 */
static void set_clicked(ui_pbutton_t *button, void *arg)
{
	date_cfg_t *date_cfg = (date_cfg_t *) arg;
	errno_t rc;

	rc = parse_date_time(date_cfg);
	if (rc != EOK) {
		printf("Error parsing date/time. Please use format DD/MM/YYYY and HH:MM:SS\n");
		return;
	}

	rc = set_system_time(&date_cfg->current_time);
	if (rc != EOK) {
		printf("Error setting system time.\n");
		return;
	}

	update_time_display(date_cfg);
}

/** Get current system time.
 *
 * @param t Pointer to tm structure to store time
 * @return EOK on success, error code otherwise
 */
static errno_t get_current_time(struct tm *t)
{
	category_id_t cat_id;
	service_id_t *svc_ids = NULL;
	size_t svc_cnt;
	service_id_t svc_id;
	char *svc_name = NULL;
	errno_t rc;

	rc = loc_category_get_id("clock", &cat_id, IPC_FLAG_BLOCKING);
	if (rc != EOK)
		return rc;

	rc = loc_category_get_svcs(cat_id, &svc_ids, &svc_cnt);
	if (rc != EOK)
		return rc;

	if (svc_cnt == 0) {
		free(svc_ids);
		return ENOENT;
	}

	rc = loc_service_get_name(svc_ids[0], &svc_name);
	if (rc != EOK) {
		free(svc_ids);
		return rc;
	}

	rc = loc_service_get_id(svc_name, &svc_id, 0);
	if (rc != EOK) {
		free(svc_name);
		free(svc_ids);
		return rc;
	}

	async_sess_t *sess = loc_service_connect(svc_id, INTERFACE_DDF, 0);
	if (!sess) {
		free(svc_name);
		free(svc_ids);
		return EIO;
	}

	rc = clock_dev_time_get(sess, t);

	free(svc_name);
	free(svc_ids);
	return rc;
}

/** Set system time.
 *
 * @param t Pointer to tm structure containing new time
 * @return EOK on success, error code otherwise
 */
static errno_t set_system_time(struct tm *t)
{
	category_id_t cat_id;
	service_id_t *svc_ids = NULL;
	size_t svc_cnt;
	service_id_t svc_id;
	char *svc_name = NULL;
	errno_t rc;

	rc = loc_category_get_id("clock", &cat_id, IPC_FLAG_BLOCKING);
	if (rc != EOK)
		return rc;

	rc = loc_category_get_svcs(cat_id, &svc_ids, &svc_cnt);
	if (rc != EOK)
		return rc;

	if (svc_cnt == 0) {
		free(svc_ids);
		return ENOENT;
	}

	rc = loc_service_get_name(svc_ids[0], &svc_name);
	if (rc != EOK) {
		free(svc_ids);
		return rc;
	}

	rc = loc_service_get_id(svc_name, &svc_id, 0);
	if (rc != EOK) {
		free(svc_name);
		free(svc_ids);
		return rc;
	}

	async_sess_t *sess = loc_service_connect(svc_id, INTERFACE_DDF, 0);
	if (!sess) {
		free(svc_name);
		free(svc_ids);
		return EIO;
	}

	rc = clock_dev_time_set(sess, t);

	free(svc_name);
	free(svc_ids);
	return rc;
}

/** Update time display entry fields.
 *
 * @param date_cfg Date configuration structure
 */
static void update_time_display(date_cfg_t *date_cfg)
{
	char date_str[32];
	char time_str[32];

	snprintf(date_str, sizeof(date_str), "%02d/%02d/%d",
	    date_cfg->current_time.tm_mday,
	    date_cfg->current_time.tm_mon + 1,
	    1900 + date_cfg->current_time.tm_year);

	snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d",
	    date_cfg->current_time.tm_hour,
	    date_cfg->current_time.tm_min,
	    date_cfg->current_time.tm_sec);

	ui_entry_set_text(date_cfg->date_entry, date_str);
	ui_entry_set_text(date_cfg->time_entry, time_str);
}

/** Parse date and time from entry fields.
 *
 * @param date_cfg Date configuration structure
 * @return EOK on success, error code otherwise
 */
static errno_t parse_date_time(date_cfg_t *date_cfg)
{
	const char *date_str = ui_entry_get_text(date_cfg->date_entry);
	const char *time_str = ui_entry_get_text(date_cfg->time_entry);
	int first_num, second_num, year;
	int hour, min, sec;

	if (sscanf(date_str, "%d/%d/%d", &first_num, &second_num, &year) != 3)
		return EINVAL;

	if (sscanf(time_str, "%d:%d:%d", &hour, &min, &sec) != 3)
		return EINVAL;

	/* Determine format based on first number */
	if (first_num > 12) {
		/* First number is day (DD/MM/YYYY format) */
		date_cfg->current_time.tm_mday = first_num;
		date_cfg->current_time.tm_mon = second_num - 1;
	} else if (second_num > 12) {
		/* Second number is day (MM/DD/YYYY format) */
		date_cfg->current_time.tm_mon = first_num - 1;
		date_cfg->current_time.tm_mday = second_num;
	} else {
		/* Ambiguous case - assume DD/MM/YYYY format */
		date_cfg->current_time.tm_mday = first_num;
		date_cfg->current_time.tm_mon = second_num - 1;
	}

	if (date_cfg->current_time.tm_mday < 1 || date_cfg->current_time.tm_mday > 31 || 
	    date_cfg->current_time.tm_mon < 0 || date_cfg->current_time.tm_mon > 11 || 
	    year < 1900)
		return EINVAL;

	if (hour < 0 || hour > 23 || min < 0 || min > 59 || sec < 0 || sec > 59)
		return EINVAL;

	date_cfg->current_time.tm_year = year - 1900;
	date_cfg->current_time.tm_hour = hour;
	date_cfg->current_time.tm_min = min;
	date_cfg->current_time.tm_sec = sec;

	return EOK;
}

/** Run Date Configuration on display server. */
static errno_t date_cfg(const char *display_spec)
{
	ui_t *ui = NULL;
	ui_wnd_params_t params;
	ui_window_t *window = NULL;
	date_cfg_t date_cfg;
	gfx_rect_t rect;
	ui_resource_t *ui_res;
	errno_t rc;

	rc = ui_create(display_spec, &ui);
	if (rc != EOK) {
		printf("Error creating UI on display %s.\n", display_spec);
		return rc;
	}

	ui_wnd_params_init(&params);
	params.caption = "Date Configuration";
	params.placement = ui_wnd_place_center;

	if (ui_is_textmode(ui)) {
		params.rect.p0.x = 0;
		params.rect.p0.y = 0;
		params.rect.p1.x = 45;
		params.rect.p1.y = 15;
	} else {
		params.rect.p0.x = 0;
		params.rect.p0.y = 0;
		params.rect.p1.x = 350;
		params.rect.p1.y = 275;
	}

	memset((void *) &date_cfg, 0, sizeof(date_cfg));
	date_cfg.ui = ui;

	rc = ui_window_create(ui, &params, &window);
	if (rc != EOK) {
		printf("Error creating window.\n");
		return rc;
	}

	ui_window_set_cb(window, &window_cb, (void *) &date_cfg);
	date_cfg.window = window;

	ui_res = ui_window_get_res(window);

	rc = ui_fixed_create(&date_cfg.fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		return rc;
	}

	/* Create date label */
	rc = ui_label_create(ui_res, "Date:", &date_cfg.date_label);
	if (rc != EOK) {
		printf("Error creating date label.\n");
		return rc;
	}

	if (ui_is_textmode(ui)) {
		rect.p0.x = 2;
		rect.p0.y = 5;
		rect.p1.x = 7;
		rect.p1.y = 6;
	} else {
		rect.p0.x = 20;
		rect.p0.y = 80;
		rect.p1.x = 100;
		rect.p1.y = 100;
	}

	ui_label_set_rect(date_cfg.date_label, &rect);
	ui_fixed_add(date_cfg.fixed, ui_label_ctl(date_cfg.date_label));

	/* Create date entry */
	rc = ui_entry_create(window, "", &date_cfg.date_entry);
	if (rc != EOK) {
		printf("Error creating date entry.\n");
		return rc;
	}

	if (ui_is_textmode(ui)) {
		rect.p0.x = 8;
		rect.p0.y = 5;
		rect.p1.x = 28;
		rect.p1.y = 6;
	} else {
		rect.p0.x = 120;
		rect.p0.y = 80;
		rect.p1.x = 250;
		rect.p1.y = 100;
	}

	ui_entry_set_rect(date_cfg.date_entry, &rect);
	ui_fixed_add(date_cfg.fixed, ui_entry_ctl(date_cfg.date_entry));

	/* Create time label */
	rc = ui_label_create(ui_res, "Time:", &date_cfg.time_label);
	if (rc != EOK) {
		printf("Error creating time label.\n");
		return rc;
	}

	if (ui_is_textmode(ui)) {
		rect.p0.x = 2;
		rect.p0.y = 7;
		rect.p1.x = 7;
		rect.p1.y = 8;
	} else {
		rect.p0.x = 20;
		rect.p0.y = 120;
		rect.p1.x = 100;
		rect.p1.y = 140;
	}

	ui_label_set_rect(date_cfg.time_label, &rect);
	ui_fixed_add(date_cfg.fixed, ui_label_ctl(date_cfg.time_label));

	/* Create time entry */
	rc = ui_entry_create(window, "", &date_cfg.time_entry);
	if (rc != EOK) {
		printf("Error creating time entry.\n");
		return rc;
	}

	if (ui_is_textmode(ui)) {
		rect.p0.x = 8;
		rect.p0.y = 7;
		rect.p1.x = 28;
		rect.p1.y = 8;
	} else {
		rect.p0.x = 120;
		rect.p0.y = 120;
		rect.p1.x = 250;
		rect.p1.y = 140;
	}

	ui_entry_set_rect(date_cfg.time_entry, &rect);
	ui_fixed_add(date_cfg.fixed, ui_entry_ctl(date_cfg.time_entry));

	/* Create Set button */
	rc = ui_pbutton_create(ui_res, "Set", &date_cfg.set_button);
	if (rc != EOK) {
		printf("Error creating Set button.\n");
		return rc;
	}

	ui_pbutton_set_cb(date_cfg.set_button, &set_button_cb, (void *) &date_cfg);

	if (ui_is_textmode(ui)) {
		rect.p0.x = 2;
		rect.p0.y = 13;
		rect.p1.x = 13;
		rect.p1.y = 14;
	} else {
		rect.p0.x = 20;
		rect.p0.y = 235;
		rect.p1.x = 120;
		rect.p1.y = rect.p0.y + 28;
	}

	ui_pbutton_set_rect(date_cfg.set_button, &rect);
	ui_fixed_add(date_cfg.fixed, ui_pbutton_ctl(date_cfg.set_button));

	/* Create OK button */
	rc = ui_pbutton_create(ui_res, "OK", &date_cfg.ok_button);
	if (rc != EOK) {
		printf("Error creating OK button.\n");
		return rc;
	}

	ui_pbutton_set_cb(date_cfg.ok_button, &button_cb, (void *) &date_cfg);

	if (ui_is_textmode(ui)) {
		rect.p0.x = 17;
		rect.p0.y = 13;
		rect.p1.x = 28;
		rect.p1.y = 14;
	} else {
		rect.p0.x = 125;
		rect.p0.y = 235;
		rect.p1.x = 225;
		rect.p1.y = rect.p0.y + 28;
	}

	ui_pbutton_set_rect(date_cfg.ok_button, &rect);
	ui_pbutton_set_default(date_cfg.ok_button, true);

	ui_fixed_add(date_cfg.fixed, ui_pbutton_ctl(date_cfg.ok_button));

	ui_window_add(window, ui_fixed_ctl(date_cfg.fixed));

	/* Get current time and update display */
	rc = get_current_time(&date_cfg.current_time);
	if (rc != EOK) {
		printf("Error getting current time.\n");
		return rc;
	}

	update_time_display(&date_cfg);

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

static void print_syntax(void)
{
	printf("Syntax: date_cfg [-d <display-spec>]\n");
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

	rc = date_cfg(display_spec);
	if (rc != EOK)
		return 1;

	return 0;
}

/** @}
 */
