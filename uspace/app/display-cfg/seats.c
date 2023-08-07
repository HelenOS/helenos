/*
 * Copyright (c) 2023 Jiri Svoboda
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

/** @addtogroup display-cfg
 * @{
 */
/** @file Seat configuration tab
 */

#include <gfx/coord.h>
#include <loc.h>
#include <stdio.h>
#include <stdlib.h>
#include <str.h>
#include <ui/control.h>
#include <ui/label.h>
#include <ui/list.h>
#include <ui/pbutton.h>
#include <ui/promptdialog.h>
#include <ui/resource.h>
#include <ui/selectdialog.h>
#include <ui/tab.h>
#include <ui/window.h>
#include "display-cfg.h"
#include "seats.h"

static errno_t dcfg_seats_asgn_dev_list_populate(dcfg_seats_t *);
static errno_t dcfg_seats_avail_dev_list_populate(dcfg_seats_t *,
    ui_select_dialog_t *);
static dcfg_seats_entry_t *dcfg_seats_get_selected(dcfg_seats_t *);
static void dcfg_seats_list_selected(ui_list_entry_t *, void *);
static void dcfg_add_seat_clicked(ui_pbutton_t *, void *);
static void dcfg_remove_seat_clicked(ui_pbutton_t *, void *);
static void dcfg_add_device_clicked(ui_pbutton_t *, void *);
static void dcfg_remove_device_clicked(ui_pbutton_t *, void *);

/** Seat list callbacks */
ui_list_cb_t dcfg_seats_list_cb = {
	.selected = dcfg_seats_list_selected
};

/** Add seat button callbacks */
ui_pbutton_cb_t dcfg_add_seat_button_cb = {
	.clicked = dcfg_add_seat_clicked
};

/** Remove seat button callbacks */
ui_pbutton_cb_t dcfg_remove_seat_button_cb = {
	.clicked = dcfg_remove_seat_clicked
};

/** Add device button callbacks */
ui_pbutton_cb_t dcfg_add_device_button_cb = {
	.clicked = dcfg_add_device_clicked
};

/** Remove device button callbacks */
ui_pbutton_cb_t dcfg_remove_device_button_cb = {
	.clicked = dcfg_remove_device_clicked
};

static void add_seat_dialog_bok(ui_prompt_dialog_t *, void *, const char *);
static void add_seat_dialog_bcancel(ui_prompt_dialog_t *, void *);
static void add_seat_dialog_close(ui_prompt_dialog_t *, void *);

/** Add seat dialog callbacks */
ui_prompt_dialog_cb_t add_seat_dialog_cb = {
	.bok = add_seat_dialog_bok,
	.bcancel = add_seat_dialog_bcancel,
	.close = add_seat_dialog_close
};

static void add_device_dialog_bok(ui_select_dialog_t *, void *, void *);
static void add_device_dialog_bcancel(ui_select_dialog_t *, void *);
static void add_device_dialog_close(ui_select_dialog_t *, void *);

/** Add device dialog callbacks */
ui_select_dialog_cb_t add_device_dialog_cb = {
	.bok = add_device_dialog_bok,
	.bcancel = add_device_dialog_bcancel,
	.close = add_device_dialog_close
};

/** Create seat configuration tab
 *
 * @param dcfg Display configuration dialog
 * @param rseats Place to store pointer to new seat configuration tab
 * @return EOK on success or an error code
 */
errno_t dcfg_seats_create(display_cfg_t *dcfg, dcfg_seats_t **rseats)
{
	ui_resource_t *ui_res;
	dcfg_seats_t *seats;
	gfx_rect_t rect;
	errno_t rc;

	ui_res = ui_window_get_res(dcfg->window);

	seats = calloc(1, sizeof(dcfg_seats_t));
	if (dcfg == NULL) {
		printf("Out of memory.\n");
		return ENOMEM;
	}

	seats->dcfg = dcfg;

	/* 'Seats' tab */

	rc = ui_tab_create(dcfg->tabset, "Seats", &seats->tab);
	if (rc != EOK)
		goto error;

	rc = ui_fixed_create(&seats->fixed);
	if (rc != EOK) {
		printf("Error creating fixed layout.\n");
		goto error;
	}

	/* 'Configured seats:' label */

	rc = ui_label_create(ui_res, "Configured seats:", &seats->seats_label);
	if (rc != EOK) {
		printf("Error creating label.\n");
		goto error;
	}

	if (ui_resource_is_textmode(ui_res)) {
		rect.p0.x = 4;
		rect.p0.y = 4;
		rect.p1.x = 36;
		rect.p1.y = 5;
	} else {
		rect.p0.x = 20;
		rect.p0.y = 60;
		rect.p1.x = 360;
		rect.p1.y = 80;
	}

	ui_label_set_rect(seats->seats_label, &rect);

	rc = ui_fixed_add(seats->fixed, ui_label_ctl(seats->seats_label));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	/* List of seats */

	rc = ui_list_create(dcfg->window, false, &seats->seat_list);
	if (rc != EOK) {
		printf("Error creating list.\n");
		goto error;
	}

	if (ui_resource_is_textmode(ui_res)) {
		rect.p0.x = 4;
		rect.p0.y = 5;
		rect.p1.x = 56;
		rect.p1.y = 10;
	} else {
		rect.p0.x = 20;
		rect.p0.y = 80;
		rect.p1.x = 360;
		rect.p1.y = 180;
	}

	ui_list_set_rect(seats->seat_list, &rect);

	rc = ui_fixed_add(seats->fixed, ui_list_ctl(seats->seat_list));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	ui_list_set_cb(seats->seat_list, &dcfg_seats_list_cb, (void *)seats);

	/* 'Add...' seat button */

	rc = ui_pbutton_create(ui_res, "Add...", &seats->add_seat);
	if (rc != EOK) {
		printf("Error creating button.\n");
		goto error;
	}

	if (ui_resource_is_textmode(ui_res)) {
		rect.p0.x = 58;
		rect.p0.y = 5;
		rect.p1.x = 68;
		rect.p1.y = 6;
	} else {
		rect.p0.x = 370;
		rect.p0.y = 80;
		rect.p1.x = 450;
		rect.p1.y = 105;
	}

	ui_pbutton_set_rect(seats->add_seat, &rect);

	rc = ui_fixed_add(seats->fixed, ui_pbutton_ctl(seats->add_seat));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	ui_pbutton_set_cb(seats->add_seat, &dcfg_add_seat_button_cb,
	    (void *)seats);

	/* 'Remove' seat button */

	rc = ui_pbutton_create(ui_res, "Remove", &seats->remove_seat);
	if (rc != EOK) {
		printf("Error creating button.\n");
		goto error;
	}

	if (ui_resource_is_textmode(ui_res)) {
		rect.p0.x = 58;
		rect.p0.y = 7;
		rect.p1.x = 68;
		rect.p1.y = 8;
	} else {
		rect.p0.x = 370;
		rect.p0.y = 110;
		rect.p1.x = 450;
		rect.p1.y = 135;
	}

	ui_pbutton_set_rect(seats->remove_seat, &rect);

	rc = ui_fixed_add(seats->fixed, ui_pbutton_ctl(seats->remove_seat));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	ui_pbutton_set_cb(seats->remove_seat, &dcfg_remove_seat_button_cb,
	    (void *)seats);

	/* 'Devices assigned to seat 'xxx':' label */

	rc = ui_label_create(ui_res, "Devices assigned to seat 'xxx':",
	    &seats->devices_label);
	if (rc != EOK) {
		printf("Error creating label.\n");
		goto error;
	}

	if (ui_resource_is_textmode(ui_res)) {
		rect.p0.x = 4;
		rect.p0.y = 11;
		rect.p1.x = 36;
		rect.p1.y = 12;
	} else {
		rect.p0.x = 20;
		rect.p0.y = 200;
		rect.p1.x = 360;
		rect.p1.y = 220;
	}

	ui_label_set_rect(seats->devices_label, &rect);

	rc = ui_fixed_add(seats->fixed, ui_label_ctl(seats->devices_label));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	/* List of devices */

	rc = ui_list_create(dcfg->window, false, &seats->device_list);
	if (rc != EOK) {
		printf("Error creating list.\n");
		goto error;
	}

	if (ui_resource_is_textmode(ui_res)) {
		rect.p0.x = 4;
		rect.p0.y = 12;
		rect.p1.x = 56;
		rect.p1.y = 17;
	} else {
		rect.p0.x = 20;
		rect.p0.y = 220;
		rect.p1.x = 360;
		rect.p1.y = 320;
	}

	ui_list_set_rect(seats->device_list, &rect);

	rc = ui_fixed_add(seats->fixed, ui_list_ctl(seats->device_list));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	/* 'Add...' device button */

	rc = ui_pbutton_create(ui_res, "Add...", &seats->add_device);
	if (rc != EOK) {
		printf("Error creating button.\n");
		goto error;
	}

	if (ui_resource_is_textmode(ui_res)) {
		rect.p0.x = 58;
		rect.p0.y = 12;
		rect.p1.x = 68;
		rect.p1.y = 13;
	} else {
		rect.p0.x = 370;
		rect.p0.y = 220;
		rect.p1.x = 450;
		rect.p1.y = 245;
	}

	ui_pbutton_set_rect(seats->add_device, &rect);

	rc = ui_fixed_add(seats->fixed, ui_pbutton_ctl(seats->add_device));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	ui_pbutton_set_cb(seats->add_device, &dcfg_add_device_button_cb,
	    (void *)seats);

	/* 'Remove' device button */

	rc = ui_pbutton_create(ui_res, "Remove", &seats->remove_device);
	if (rc != EOK) {
		printf("Error creating button.\n");
		goto error;
	}

	if (ui_resource_is_textmode(ui_res)) {
		rect.p0.x = 58;
		rect.p0.y = 14;
		rect.p1.x = 68;
		rect.p1.y = 15;
	} else {
		rect.p0.x = 370;
		rect.p0.y = 250;
		rect.p1.x = 450;
		rect.p1.y = 275;
	}

	ui_pbutton_set_rect(seats->remove_device, &rect);

	rc = ui_fixed_add(seats->fixed, ui_pbutton_ctl(seats->remove_device));
	if (rc != EOK) {
		printf("Error adding control to layout.\n");
		goto error;
	}

	ui_pbutton_set_cb(seats->remove_device, &dcfg_remove_device_button_cb,
	    (void *)seats);

	ui_tab_add(seats->tab, ui_fixed_ctl(seats->fixed));

	*rseats = seats;
	return EOK;
error:
	if (seats->remove_device != NULL)
		ui_pbutton_destroy(seats->remove_device);
	if (seats->add_device != NULL)
		ui_pbutton_destroy(seats->add_device);
	if (seats->devices_label != NULL)
		ui_label_destroy(seats->devices_label);
	if (seats->device_list != NULL)
		ui_list_destroy(seats->device_list);
	if (seats->remove_seat != NULL)
		ui_pbutton_destroy(seats->remove_seat);
	if (seats->add_seat != NULL)
		ui_pbutton_destroy(seats->add_seat);
	if (seats->seats_label != NULL)
		ui_label_destroy(seats->seats_label);
	if (seats->seat_list != NULL)
		ui_list_destroy(seats->seat_list);
	if (seats->fixed != NULL)
		ui_fixed_destroy(seats->fixed);
	free(dcfg);
	return rc;
}

/** Populate seats tab with display configuration service data
 *
 * @param dcfg Display configuration dialog
 * @param rseats Place to store pointer to new seat configuration tab
 * @return EOK on success or an error code
 */
errno_t dcfg_seats_populate(dcfg_seats_t *seats)
{
	dcfg_seats_entry_t *entry;
	errno_t rc;

	rc = dcfg_seats_list_populate(seats);
	if (rc != EOK)
		return rc;

	/*
	 * Update "Devices assigned to seat 'xxx'" label and populate
	 * assigned devices list.
	 */
	entry = dcfg_seats_get_selected(seats);
	dcfg_seats_list_selected(entry->lentry, (void *)entry);
	return EOK;
}

/** Destroy display configuration dialog.
 *
 * @param dcfg Display configuration dialog
 */
void dcfg_seats_destroy(dcfg_seats_t *seats)
{
	ui_list_entry_t *lentry;
	dcfg_seats_entry_t *entry;
	dcfg_devices_entry_t *dentry;

	lentry = ui_list_first(seats->device_list);
	while (lentry != NULL) {
		dentry = (dcfg_devices_entry_t *)ui_list_entry_get_arg(lentry);
		free(dentry->name);
		free(dentry);
		ui_list_entry_delete(lentry);
		lentry = ui_list_first(seats->device_list);
	}

	lentry = ui_list_first(seats->seat_list);
	while (lentry != NULL) {
		entry = (dcfg_seats_entry_t *)ui_list_entry_get_arg(lentry);
		free(entry->name);
		free(entry);
		ui_list_entry_delete(lentry);
		lentry = ui_list_first(seats->seat_list);
	}

	/* This will automatically destroy all controls in the tab */
	ui_tab_destroy(seats->tab);
	free(seats);
}

/** Insert new entry into seats list.
 *
 * @param seats Seat configuration tab
 * @param name Seat name
 * @param seat_id Seat ID
 * @param rentry Place to store pointer to new entry or NULL
 * @return EOK on success or an error code
 */
errno_t dcfg_seats_insert(dcfg_seats_t *seats, const char *name,
    sysarg_t seat_id, dcfg_seats_entry_t **rentry)
{
	dcfg_seats_entry_t *entry;
	ui_list_entry_attr_t attr;
	errno_t rc;

	entry = calloc(1, sizeof(dcfg_seats_entry_t));
	if (entry == NULL)
		return ENOMEM;

	entry->seats = seats;
	entry->seat_id = seat_id;
	entry->name = str_dup(name);
	if (entry->name == NULL) {
		free(entry);
		return ENOMEM;
	}

	ui_list_entry_attr_init(&attr);
	attr.caption = name;
	attr.arg = (void *)entry;
	rc = ui_list_entry_append(seats->seat_list, &attr, &entry->lentry);
	if (rc != EOK) {
		free(entry->name);
		free(entry);
		return rc;
	}

	if (rentry != NULL)
		*rentry = entry;
	return EOK;
}

/** Populate seat list.
 *
 * @param seats Seat configuration tab
 * @return EOK on success or an error code
 */
errno_t dcfg_seats_list_populate(dcfg_seats_t *seats)
{
	size_t i;
	dispcfg_seat_list_t *seat_list = NULL;
	dispcfg_seat_info_t *sinfo = NULL;
	errno_t rc;

	rc = dispcfg_get_seat_list(seats->dcfg->dispcfg, &seat_list);
	if (rc != EOK)
		goto error;

	for (i = 0; i < seat_list->nseats; i++) {
		rc = dispcfg_get_seat_info(seats->dcfg->dispcfg,
		    seat_list->seats[i], &sinfo);
		if (rc != EOK)
			goto error;

		rc = dcfg_seats_insert(seats, sinfo->name, seat_list->seats[i],
		    NULL);
		if (rc != EOK)
			goto error;

		dispcfg_free_seat_info(sinfo);
		sinfo = NULL;
	}

	dispcfg_free_seat_list(seat_list);
	return EOK;
error:
	if (sinfo != NULL)
		dispcfg_free_seat_info(sinfo);
	if (seat_list != NULL)
		dispcfg_free_seat_list(seat_list);
	return rc;
}

/** Insert new entry into devices list.
 *
 * @param seats Seat configuration tab
 * @param name Device name
 * @param svc_id Service ID
 * @return EOK on success or an error code
 */
errno_t dcfg_devices_insert(dcfg_seats_t *seats, const char *name,
    service_id_t svc_id)
{
	dcfg_devices_entry_t *entry;
	ui_list_entry_attr_t attr;
	errno_t rc;

	entry = calloc(1, sizeof(dcfg_devices_entry_t));
	if (entry == NULL)
		return ENOMEM;

	entry->seats = seats;
	entry->svc_id = svc_id;
	entry->name = str_dup(name);
	if (entry->name == NULL) {
		free(entry);
		return ENOMEM;
	}

	ui_list_entry_attr_init(&attr);
	attr.caption = name;
	attr.arg = (void *)entry;
	rc = ui_list_entry_append(seats->device_list, &attr, &entry->lentry);
	if (rc != EOK) {
		free(entry->name);
		free(entry);
		return rc;
	}

	return EOK;
}

/** Insert new entry into available devices list.
 *
 * @param seats Seat configuration tab
 * @param dialog 'Add Device' dialog
 * @param name Device name
 * @param svc_id Service ID
 * @return EOK on success or an error code
 */
errno_t dcfg_avail_devices_insert(dcfg_seats_t *seats,
    ui_select_dialog_t *dialog, const char *name, service_id_t svc_id)
{
	dcfg_devices_entry_t *entry;
	ui_list_entry_attr_t attr;
	errno_t rc;

	entry = calloc(1, sizeof(dcfg_devices_entry_t));
	if (entry == NULL)
		return ENOMEM;

	entry->seats = seats;
	entry->svc_id = svc_id;
	entry->name = str_dup(name);
	if (entry->name == NULL) {
		free(entry);
		return ENOMEM;
	}

	ui_list_entry_attr_init(&attr);
	attr.caption = name;
	attr.arg = (void *)entry;
	rc = ui_select_dialog_append(dialog, &attr);
	if (rc != EOK) {
		free(entry->name);
		free(entry);
		return rc;
	}

	return EOK;
}

/** Populate assigned device list.
 *
 * @param seats Seat configuration tab
 * @return EOK on success or an error code
 */
static errno_t dcfg_seats_asgn_dev_list_populate(dcfg_seats_t *seats)
{
	size_t i;
	dispcfg_dev_list_t *dev_list = NULL;
	char *svc_name = NULL;
	dcfg_seats_entry_t *seats_entry;
	sysarg_t seat_id;
	errno_t rc;

	/* Get active seat entry */
	seats_entry = dcfg_seats_get_selected(seats);
	seat_id = seats_entry->seat_id;

	rc = dispcfg_get_asgn_dev_list(seats->dcfg->dispcfg, seat_id, &dev_list);
	if (rc != EOK)
		goto error;

	for (i = 0; i < dev_list->ndevs; i++) {
		rc = loc_service_get_name(dev_list->devs[i], &svc_name);
		if (rc != EOK)
			goto error;

		rc = dcfg_devices_insert(seats, svc_name, dev_list->devs[i]);
		if (rc != EOK)
			goto error;

		free(svc_name);
		svc_name = NULL;
	}

	dispcfg_free_dev_list(dev_list);
	return EOK;
error:
	if (svc_name != NULL)
		free(svc_name);
	if (dev_list != NULL)
		dispcfg_free_dev_list(dev_list);
	return rc;
}

/** Populate available device list in 'Add Device' dialog.
 *
 * @param seats Seat configuration tab
 * @param dialog 'Add Device' dialog
 * @return EOK on success or an error code
 */
static errno_t dcfg_seats_avail_dev_list_populate(dcfg_seats_t *seats,
    ui_select_dialog_t *dialog)
{
	size_t i, j, k;
	category_id_t cat_id;
	service_id_t *kbd_svcs = NULL;
	size_t nkbd_svcs;
	service_id_t *mouse_svcs = NULL;
	size_t nmouse_svcs;
	dispcfg_seat_list_t *seat_list = NULL;
	dispcfg_dev_list_t *adev_list;
	char *svc_name = NULL;
	errno_t rc;

	/* Get list of keyboard devices */

	rc = loc_category_get_id("keyboard", &cat_id, 0);
	if (rc != EOK) {
		printf("Error getting category ID.\n");
		goto error;
	}

	rc = loc_category_get_svcs(cat_id, &kbd_svcs, &nkbd_svcs);
	if (rc != EOK) {
		printf("Error getting service list.\n");
		goto error;
	}

	/* Get list of mouse devices */

	rc = loc_category_get_id("mouse", &cat_id, 0);
	if (rc != EOK) {
		printf("Error getting category ID.\n");
		goto error;
	}

	rc = loc_category_get_svcs(cat_id, &mouse_svcs, &nmouse_svcs);
	if (rc != EOK) {
		printf("Error getting service list.\n");
		goto error;
	}

	/* Filter out assigned devices */
	rc = dispcfg_get_seat_list(seats->dcfg->dispcfg, &seat_list);
	if (rc != EOK) {
		printf("Error getting seat list.\n");
		goto error;
	}

	for (i = 0; i < seat_list->nseats; i++) {
		rc = dispcfg_get_asgn_dev_list(seats->dcfg->dispcfg,
		    seat_list->seats[i], &adev_list);
		if (rc != EOK) {
			printf("Error getting device list.\n");
			goto error;
		}

		for (j = 0; j < adev_list->ndevs; j++) {
			/* Filter out assigned keyboard devices */
			for (k = 0; k < nkbd_svcs; k++) {
				if (kbd_svcs[k] == adev_list->devs[j])
					kbd_svcs[k] = 0;
			}

			/* Filter out assigned mouse devices */
			for (k = 0; k < nmouse_svcs; k++) {
				if (mouse_svcs[k] == adev_list->devs[j])
					mouse_svcs[k] = 0;
			}
		}

		dispcfg_free_dev_list(adev_list);
	}

	dispcfg_free_seat_list(seat_list);
	seat_list = NULL;

	/* Add keyboard devices */

	for (i = 0; i < nkbd_svcs; i++) {
		if (kbd_svcs[i] == 0)
			continue;

		rc = loc_service_get_name(kbd_svcs[i], &svc_name);
		if (rc != EOK)
			goto error;

		rc = dcfg_avail_devices_insert(seats, dialog, svc_name,
		    kbd_svcs[i]);
		if (rc != EOK)
			goto error;

		free(svc_name);
		svc_name = NULL;
	}

	/* Add mouse devices */

	for (i = 0; i < nmouse_svcs; i++) {
		if (mouse_svcs[i] == 0)
			continue;

		rc = loc_service_get_name(mouse_svcs[i], &svc_name);
		if (rc != EOK)
			goto error;

		rc = dcfg_avail_devices_insert(seats, dialog, svc_name,
		    mouse_svcs[i]);
		if (rc != EOK)
			goto error;

		free(svc_name);
		svc_name = NULL;
	}

	free(kbd_svcs);
	free(mouse_svcs);
	return EOK;
error:
	if (svc_name != NULL)
		free(svc_name);
	if (seat_list != NULL)
		dispcfg_free_seat_list(seat_list);
	if (kbd_svcs != NULL)
		free(kbd_svcs);
	if (mouse_svcs != NULL)
		free(mouse_svcs);
	return rc;
}

/** Get selected seat entry.
 *
 * @param seats Seat configuration tab
 * @return Selected entry
 */
static dcfg_seats_entry_t *dcfg_seats_get_selected(dcfg_seats_t *seats)
{
	ui_list_entry_t *lentry;

	lentry = ui_list_get_cursor(seats->seat_list);
	return (dcfg_seats_entry_t *)ui_list_entry_get_arg(lentry);
}

/** Get selected device entry.
 *
 * @param seats Seat configuration tab
 * @return Selected entry
 */
static dcfg_devices_entry_t *dcfg_devices_get_selected(dcfg_seats_t *seats)
{
	ui_list_entry_t *lentry;

	lentry = ui_list_get_cursor(seats->device_list);
	return (dcfg_devices_entry_t *)ui_list_entry_get_arg(lentry);
}

/** Entry in seats list is selected.
 *
 * @param lentry UI list entry
 * @param arg Argument (dcfg_seats_entry_t *)
 */
static void dcfg_seats_list_selected(ui_list_entry_t *lentry, void *arg)
{
	dcfg_seats_entry_t *entry = (dcfg_seats_entry_t *)arg;
	dcfg_devices_entry_t *dentry;
	ui_list_entry_t *le;
	char *caption;
	errno_t rc;
	int rv;

	(void) lentry;

	/* Update 'Devices assigned to seat 'xxx':' label */

	rv = asprintf(&caption, "Devices assigned to seat '%s':", entry->name);
	if (rv < 0) {
		printf("Out of memory.\n");
		return;
	}

	rc = ui_label_set_text(entry->seats->devices_label, caption);
	if (rc != EOK) {
		printf("Error setting label.\n");
		return;
	}

	free(caption);

	(void) ui_control_paint(ui_label_ctl(entry->seats->devices_label));

	/* Clear device list */
	le = ui_list_first(entry->seats->device_list);
	while (le != NULL) {
		dentry = (dcfg_devices_entry_t *)ui_list_entry_get_arg(le);
		free(dentry->name);
		free(dentry);
		ui_list_entry_delete(le);
		le = ui_list_first(entry->seats->device_list);
	}

	/* Re-populate it */
	(void) dcfg_seats_asgn_dev_list_populate(entry->seats);
	(void) ui_control_paint(ui_list_ctl(entry->seats->device_list));
}

/** "Add' seat button clicked.
 *
 * @param pbutton Push button
 * @param arg Argument (dcfg_seats_entry_t *)
 */
static void dcfg_add_seat_clicked(ui_pbutton_t *pbutton, void *arg)
{
	dcfg_seats_t *seats = (dcfg_seats_t *)arg;
	ui_prompt_dialog_params_t pdparams;
	errno_t rc;

	ui_prompt_dialog_params_init(&pdparams);
	pdparams.caption = "Add Seat";
	pdparams.prompt = "New Seat Name";

	rc = ui_prompt_dialog_create(seats->dcfg->ui, &pdparams,
	    &seats->add_seat_dlg);
	if (rc != EOK)
		printf("Error creating dialog.\n");

	ui_prompt_dialog_set_cb(seats->add_seat_dlg, &add_seat_dialog_cb,
	    (void *)seats);
}

/** "Remove' seat button clicked.
 *
 * @param pbutton Push button
 * @param arg Argument (dcfg_seats_entry_t *)
 */
static void dcfg_remove_seat_clicked(ui_pbutton_t *pbutton, void *arg)
{
	dcfg_seats_t *seats = (dcfg_seats_t *)arg;
	dcfg_seats_entry_t *entry;
	errno_t rc;

	(void)pbutton;
	entry = dcfg_seats_get_selected(seats);

	rc = dispcfg_seat_delete(seats->dcfg->dispcfg, entry->seat_id);
	if (rc != EOK) {
		/*
		 * EBUSY is returned when we attempt to delete the last
		 * seat. No need to complain about it.
		 */
		if (rc == EBUSY)
			return;

		printf("Error removing seat '%s'.\n", entry->name);
		return;
	}

	ui_list_entry_delete(entry->lentry);
	free(entry->name);
	free(entry);

	(void) ui_control_paint(ui_list_ctl(seats->seat_list));

	/* Since selected seat changed we need to update device list */
	entry = dcfg_seats_get_selected(seats);
	dcfg_seats_list_selected(entry->lentry, (void *)entry);
}

/** Add seat dialog OK button was pressed.
 *
 * @param dialog Add seat dialog
 * @param arg Argument (dcfg_seats_t *)
 * @param text Submitted text
 */
void add_seat_dialog_bok(ui_prompt_dialog_t *dialog, void *arg,
    const char *text)
{
	dcfg_seats_t *seats = (dcfg_seats_t *)arg;
	sysarg_t seat_id;
	dcfg_seats_entry_t *entry;
	errno_t rc;

	seats->add_seat_dlg = NULL;
	ui_prompt_dialog_destroy(dialog);

	rc = dispcfg_seat_create(seats->dcfg->dispcfg, text, &seat_id);
	if (rc != EOK) {
		printf("Error creating seat '%s'.\n", text);
		return;
	}

	rc = dcfg_seats_insert(seats, text, seat_id, &entry);
	if (rc != EOK)
		return;

	(void) ui_control_paint(ui_list_ctl(seats->seat_list));

	/* Select new seat and update device list */
	ui_list_set_cursor(seats->seat_list, entry->lentry);
	dcfg_seats_list_selected(entry->lentry, (void *)entry);
}

/** Add seat dialog Cancel button was pressed.
 *
 * @param dialog Add seat dialog
 * @param arg Argument (dcfg_seats_t *)
 */
void add_seat_dialog_bcancel(ui_prompt_dialog_t *dialog, void *arg)
{
	dcfg_seats_t *seats = (dcfg_seats_t *)arg;

	seats->add_seat_dlg = NULL;
	ui_prompt_dialog_destroy(dialog);
}

/** Add seat dialog close request.
 *
 * @param dialog Add seat dialog
 * @param arg Argument (dcfg_seats_t *)
 */
void add_seat_dialog_close(ui_prompt_dialog_t *dialog, void *arg)
{
	dcfg_seats_t *seats = (dcfg_seats_t *)arg;

	seats->add_seat_dlg = NULL;
	ui_prompt_dialog_destroy(dialog);
}

/** "Add' device button clicked.
 *
 * @param pbutton Push button
 * @param arg Argument (dcfg_seats_entry_t *)
 */
static void dcfg_add_device_clicked(ui_pbutton_t *pbutton, void *arg)
{
	dcfg_seats_t *seats = (dcfg_seats_t *)arg;
	ui_select_dialog_params_t sdparams;
	errno_t rc;

	ui_select_dialog_params_init(&sdparams);
	sdparams.caption = "Add Device";
	sdparams.prompt = "Device Name";

	rc = ui_select_dialog_create(seats->dcfg->ui, &sdparams,
	    &seats->add_device_dlg);
	if (rc != EOK)
		printf("Error creating dialog.\n");

	ui_select_dialog_set_cb(seats->add_device_dlg, &add_device_dialog_cb,
	    (void *)seats);

	(void) dcfg_seats_avail_dev_list_populate(seats,
	    seats->add_device_dlg);
	ui_select_dialog_paint(seats->add_device_dlg);
}

/** "Remove' device button clicked.
 *
 * @param pbutton Push button
 * @param arg Argument (dcfg_seats_entry_t *)
 */
static void dcfg_remove_device_clicked(ui_pbutton_t *pbutton, void *arg)
{
	dcfg_seats_t *seats = (dcfg_seats_t *)arg;
	dcfg_devices_entry_t *entry;
	errno_t rc;

	(void)pbutton;
	entry = dcfg_devices_get_selected(seats);

	rc = dispcfg_dev_unassign(seats->dcfg->dispcfg, entry->svc_id);
	if (rc != EOK) {
		printf("Error removing device '%s'.\n", entry->name);
		return;
	}

	ui_list_entry_delete(entry->lentry);
	free(entry->name);
	free(entry);

	(void) ui_control_paint(ui_list_ctl(seats->device_list));
}

/** Add device dialog OK button was pressed.
 *
 * @param dialog Add device dialog
 * @param arg Argument (dcfg_seats_t *)
 * @param text Submitted text
 */
void add_device_dialog_bok(ui_select_dialog_t *dialog, void *arg,
    void *earg)
{
	dcfg_seats_t *seats = (dcfg_seats_t *)arg;
	dcfg_devices_entry_t *entry;
	dcfg_seats_entry_t *seat;
	errno_t rc;

	seat = dcfg_seats_get_selected(seats);
	entry = (dcfg_devices_entry_t *)earg;

	seats->add_device_dlg = NULL;
	ui_select_dialog_destroy(dialog);

	rc = dispcfg_dev_assign(seats->dcfg->dispcfg, entry->svc_id,
	    seat->seat_id);
	if (rc != EOK) {
		printf("Error assigning device '%s' seat '%s'.\n",
		    entry->name, seat->name);
		return;
	}

	rc = dcfg_devices_insert(seats, entry->name, entry->svc_id);
	if (rc != EOK) {
		printf("Error inserting device to list.\n");
		return;
	}

	(void) ui_control_paint(ui_list_ctl(seats->device_list));
}

/** Add device dialog Cancel button was pressed.
 *
 * @param dialog Add device dialog
 * @param arg Argument (dcfg_seats_t *)
 */
void add_device_dialog_bcancel(ui_select_dialog_t *dialog, void *arg)
{
	dcfg_seats_t *seats = (dcfg_seats_t *)arg;

	seats->add_device_dlg = NULL;
	ui_select_dialog_destroy(dialog);
}

/** Add device dialog close request.
 *
 * @param dialog Add device dialog
 * @param arg Argument (dcfg_seats_t *)
 */
void add_device_dialog_close(ui_select_dialog_t *dialog, void *arg)
{
	dcfg_seats_t *seats = (dcfg_seats_t *)arg;

	seats->add_device_dlg = NULL;
	ui_select_dialog_destroy(dialog);
}

/** @}
 */
