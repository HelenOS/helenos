/*
 * Copyright (c) 2025 Matej Volf
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

#include <fibril.h>
#include <pcut/pcut.h>

PCUT_INIT;

PCUT_TEST_SUITE(fibril_exit_hook);

static int value;

static void exit_hook(void)
{
	value = 5;
}

static errno_t fibril_basic(void *_arg)
{
	fibril_add_exit_hook(exit_hook);
	return EOK;
}

PCUT_TEST(exit_hook_basic)
{
	value = 0;
	fid_t other = fibril_create(fibril_basic, NULL);
	fibril_start(other);

	fibril_yield();

	PCUT_ASSERT_INT_EQUALS(5, value);
}

/*
 * static errno_t fibril_to_be_killed(void* _arg) {
 *     fibril_add_exit_hook(exit_hook);
 *
 *     while (true)
 *         firbil_yield();
 *
 *     assert(0 && "unreachable");
 * }
 *
 * PCUT_TEST(exit_hook_kill) {
 *     value = 0;
 *     fid_t other = fibril_create(fibril_to_be_killed, NULL);
 *     fibril_start(other);
 *
 *     fibril_yield();
 *
 *     fibril_kill(other); // anything like this doesn't exist yet
 *
 *     PCUT_ASSERT_INT_EQUALS(5, value);
 * }
 */

PCUT_EXPORT(fibril_exit_hook);
