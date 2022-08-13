/*
 * SPDX-FileCopyrightText: 2019 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <pcut/pcut.h>
#include <str.h>
#include "../msg.h"

PCUT_INIT;

PCUT_TEST_SUITE(msg);

/** Test creating and deleting dummy message */
PCUT_TEST(new_delete)
{
	udp_msg_t *msg;

	msg = udp_msg_new();
	udp_msg_delete(msg);
}

/** Test creating, filling in and deleting message */
PCUT_TEST(new_fill_in_delete)
{
	udp_msg_t *msg;
	const char *msgstr = "Hello";

	msg = udp_msg_new();
	PCUT_ASSERT_NOT_NULL(msg);
	msg->data_size = str_size(msgstr) + 1;
	msg->data = str_dup(msgstr);

	udp_msg_delete(msg);
}

PCUT_EXPORT(msg);
