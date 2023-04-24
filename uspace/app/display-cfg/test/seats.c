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

//??? Requires us to create a test display config service
PCUT_TEST(seats_list_populate)
{
	errno_t rc;
	service_id_t sid;
	dispcfg_t *dispcfg = NULL;
	test_response_t resp;

	async_set_fallback_port_handler(test_dispcfg_conn, &resp);

	// FIXME This causes this test to be non-reentrant!
	rc = loc_server_register(test_dispcfg_server);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_register(test_dispcfg_svc, &sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = dispcfg_open(test_dispcfg_svc, NULL, NULL, &dispcfg);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(dispcfg);

	dispcfg_close(dispcfg);
	rc = loc_service_unregister(sid);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
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

PCUT_TEST(avail_devices_insert)
{
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
