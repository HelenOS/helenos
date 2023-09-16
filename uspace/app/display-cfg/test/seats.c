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

#include <async.h>
#include <dispcfg.h>
#include <dispcfg_srv.h>
#include <errno.h>
#include <pcut/pcut.h>
#include <str.h>
#include <testdc.h>
#include "../display-cfg.h"
#include "../seats.h"

PCUT_INIT;

PCUT_TEST_SUITE(seats);

static const char *test_dispcfg_server = "test-dispcfg";
static const char *test_dispcfg_svc = "test/dispcfg";

/** Test dcfg_seats_create() and dcfg_seats_destroy() */
PCUT_TEST(create_destroy)
{
	display_cfg_t *dcfg;
	dcfg_seats_t *seats;
	errno_t rc;

	rc = display_cfg_create(UI_DISPLAY_NULL, &dcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dcfg_seats_create(dcfg, &seats);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	dcfg_seats_destroy(seats);
	display_cfg_destroy(dcfg);
}

/** dcfg_seats_insert() inserts an entry into the seat list */
PCUT_TEST(seats_insert)
{
	display_cfg_t *dcfg;
	dcfg_seats_t *seats;
	dcfg_seats_entry_t *entry = NULL;
	errno_t rc;

	rc = display_cfg_create(UI_DISPLAY_NULL, &dcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dcfg_seats_create(dcfg, &seats);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dcfg_seats_insert(seats, "Alice", 42, &entry);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(entry);

	PCUT_ASSERT_STR_EQUALS("Alice", entry->name);
	PCUT_ASSERT_INT_EQUALS(42, entry->seat_id);

	dcfg_seats_destroy(seats);
	display_cfg_destroy(dcfg);
}

/** dcfg_seats_list_populate() populates seat list */
PCUT_TEST(seats_list_populate)
{
	display_cfg_t *dcfg;
	dcfg_seats_t *seats;
	errno_t rc;
	service_id_t sid;
	test_response_t resp;
	loc_srv_t *srv;

	async_set_fallback_port_handler(test_dispcfg_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_dispcfg_server, &srv);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(srv, test_dispcfg_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_cfg_create(UI_DISPLAY_NULL, &dcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = display_cfg_open(dcfg, test_dispcfg_svc);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dcfg_seats_create(dcfg, &seats);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	/*
	 * dcfg_seat_list_populate() calls dispcfg_get_seat_list()
	 * and dispcfg_get_seat_info()
	 */
	resp.rc = EOK;
	resp.get_seat_list_rlist = calloc(1, sizeof(dispcfg_seat_list_t));
	PCUT_ASSERT_NOT_NULL(resp.get_seat_list_rlist);
	resp.get_seat_list_rlist->nseats = 1;
	resp.get_seat_list_rlist->seats = calloc(1, sizeof(sysarg_t));
	PCUT_ASSERT_NOT_NULL(resp.get_seat_list_rlist->seats);
	resp.get_seat_list_rlist->seats[0] = 42;

	resp.get_seat_info_rinfo = calloc(1, sizeof(dispcfg_seat_info_t));
	PCUT_ASSERT_NOT_NULL(resp.get_seat_info_rinfo);
	resp.get_seat_info_rinfo->name = str_dup("Alice");
	PCUT_ASSERT_NOT_NULL(resp.get_seat_info_rinfo->name);

	rc = dcfg_seats_list_populate(seats);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	dcfg_seats_destroy(seats);
	display_cfg_destroy(dcfg);

	rc = loc_service_unregister(srv, sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	loc_server_unregister(srv);
}

/** dcfg_devices_insert() inserts an entry into the device list */
PCUT_TEST(devices_insert)
{
	display_cfg_t *dcfg;
	dcfg_seats_t *seats;
	ui_list_entry_t *lentry;
	dcfg_devices_entry_t *entry;
	errno_t rc;

	rc = display_cfg_create(UI_DISPLAY_NULL, &dcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dcfg_seats_create(dcfg, &seats);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dcfg_devices_insert(seats, "mydevice", 42);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	lentry = ui_list_first(seats->device_list);
	PCUT_ASSERT_NOT_NULL(lentry);
	entry = (dcfg_devices_entry_t *)ui_list_entry_get_arg(lentry);
	PCUT_ASSERT_NOT_NULL(entry);

	PCUT_ASSERT_STR_EQUALS("mydevice", entry->name);
	PCUT_ASSERT_INT_EQUALS(42, entry->svc_id);

	dcfg_seats_destroy(seats);
	display_cfg_destroy(dcfg);
}

/** dcfg_avail_devices_insert() inserts entry into available devices list */
PCUT_TEST(avail_devices_insert)
{
	display_cfg_t *dcfg;
	dcfg_seats_t *seats;
	ui_list_entry_t *lentry;
	dcfg_devices_entry_t *entry;
	ui_select_dialog_t *dialog;
	ui_select_dialog_params_t sdparams;
	errno_t rc;

	rc = display_cfg_create(UI_DISPLAY_NULL, &dcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dcfg_seats_create(dcfg, &seats);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	ui_select_dialog_params_init(&sdparams);
	sdparams.caption = "Dialog";
	sdparams.prompt = "Select";

	rc = ui_select_dialog_create(seats->dcfg->ui, &sdparams, &dialog);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dcfg_avail_devices_insert(seats, dialog, "mydevice", 42);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	lentry = ui_list_first(ui_select_dialog_list(dialog));
	PCUT_ASSERT_NOT_NULL(lentry);
	entry = (dcfg_devices_entry_t *)ui_list_entry_get_arg(lentry);
	PCUT_ASSERT_NOT_NULL(entry);

	PCUT_ASSERT_STR_EQUALS("mydevice", entry->name);
	PCUT_ASSERT_INT_EQUALS(42, entry->svc_id);

	ui_select_dialog_destroy(dialog);
	dcfg_seats_destroy(seats);
	display_cfg_destroy(dcfg);
}

PCUT_TEST(asgn_dev_list_populate)
{
}

PCUT_TEST(avail_dev_list_populate)
{
}

PCUT_TEST(seats_get_selected)
{
}

PCUT_TEST(devices_get_selected)
{
}

PCUT_TEST(seats_list_selected)
{
}

PCUT_TEST(add_seat_clicked)
{
}

PCUT_TEST(remove_seat_clicked)
{
}

PCUT_TEST(add_seat_dialog_bok)
{
}

PCUT_TEST(add_seat_dialog_bcancel)
{
}

PCUT_TEST(add_seat_dialog_close)
{
}

PCUT_TEST(add_device_clicked)
{
}

PCUT_TEST(remove_device_clicked)
{
}

PCUT_TEST(add_device_dialog_bok)
{
}

PCUT_TEST(add_device_dialog_bcancel)
{
}

PCUT_TEST(add_device_dialog_close)
{
}

PCUT_EXPORT(seats);
