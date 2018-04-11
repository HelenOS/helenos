/*
 * Copyright (c) 2014 Vojtech Horky
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
#include "tested.h"

PCUT_INIT;

static int init_counter = 1;

static void init_hook(void)
{
	init_counter++;
}

static void pre_init_hook(int *argc, char **argv[])
{
	(void) argc;
	(void) argv;
	init_counter *= 2;
}

PCUT_TEST_BEFORE
{
	PCUT_ASSERT_INT_EQUALS(4, init_counter);
	init_counter++;
}

PCUT_TEST(check_init_counter)
{
	PCUT_ASSERT_INT_EQUALS(5, init_counter);
}

PCUT_TEST(check_init_counter_2)
{
	PCUT_ASSERT_INT_EQUALS(5, init_counter);
}


PCUT_CUSTOM_MAIN(PCUT_MAIN_SET_INIT_HOOK(init_hook),
    PCUT_MAIN_SET_PREINIT_HOOK(pre_init_hook));

