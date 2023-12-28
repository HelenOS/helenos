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

#include <errno.h>
#include <loc.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(loc);

/** loc_server_register() can be called multiple times */
PCUT_TEST(server_register)
{
	errno_t rc;
	loc_srv_t *sa, *sb;
	service_id_t svca, svcb;
	char *na, *nb;
	char *sna, *snb;

	rc = loc_server_register("test-a", &sa);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_server_register("test-b", &sb);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	// XXX Without a unique name this is not reentrant
	rc = loc_service_register(sa, "test/libc-service-a", &svca);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	// XXX Without a unique name this is not reentrant
	rc = loc_service_register(sb, "test/libc-service-b", &svcb);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	rc = loc_service_get_name(svca, &na);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_STR_EQUALS("test/libc-service-a", na);

	rc = loc_service_get_server_name(svca, &sna);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_STR_EQUALS("test-a", sna);

	rc = loc_service_get_name(svcb, &nb);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_STR_EQUALS("test/libc-service-b", nb);

	rc = loc_service_get_server_name(svcb, &snb);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	PCUT_ASSERT_STR_EQUALS("test-b", snb);

	rc = loc_service_unregister(sa, svca);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);
	rc = loc_service_unregister(sb, svcb);
	PCUT_ASSERT_ERRNO_VAL(EOK, rc);

	loc_server_unregister(sa);
	loc_server_unregister(sb);
}

PCUT_EXPORT(loc);
