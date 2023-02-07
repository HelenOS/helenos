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

#include <pcut/pcut.h>
#include <ui/control.h>
#include <ui/testctl.h>
#include "../private/testctl.h"

PCUT_INIT;

PCUT_TEST_SUITE(testctl);

/** Create and destroy test control */
PCUT_TEST(create_destroy)
{
	ui_test_ctl_t *testctl = NULL;
	ui_tc_resp_t resp;
	errno_t rc;

	rc = ui_test_ctl_create(&resp, &testctl);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_NOT_NULL(testctl);

	ui_test_ctl_destroy(testctl);
}

/** ui_test_ctl_destroy() can take NULL argument (no-op) */
PCUT_TEST(destroy_null)
{
	ui_test_ctl_destroy(NULL);
}

/** ui_test_ctl_ctl() returns control that has a working virtual destructor */
PCUT_TEST(ctl)
{
	ui_control_t *control;
	ui_test_ctl_t *testctl;
	errno_t rc;
	ui_tc_resp_t resp;

	rc = ui_test_ctl_create(&resp, &testctl);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	control = ui_test_ctl_ctl(testctl);
	PCUT_ASSERT_NOT_NULL(control);

	ui_control_destroy(control);
}

PCUT_EXPORT(testctl);
