/*
 * Copyright (c) 2012-2018 Vojtech Horky
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

#ifdef __helenos__
#include <fibril.h>
#else
#if defined(__unix) || defined(__APPLE__)
#include <unistd.h>
#endif
#if defined(__WIN64) || defined(__WIN32) || defined(_WIN32)
#include <windows.h>
#endif
#endif

#include <stdio.h>
#include "tested.h"

static void my_sleep(int sec) {
#ifdef __helenos__
	fibril_sleep(sec);
#else
#if defined(__unix) || defined(__APPLE__)
	sleep(sec);
#endif
#if defined(__WIN64) || defined(__WIN32) || defined(_WIN32)
	Sleep(1000 * sec);
#endif
#endif
}

PCUT_INIT

PCUT_TEST(shall_time_out) {
	printf("Text before sleeping.\n");
	my_sleep(PCUT_DEFAULT_TEST_TIMEOUT * 5);
	printf("Text after the sleep.\n");
}

PCUT_TEST(custom_time_out,
		PCUT_TEST_SET_TIMEOUT(PCUT_DEFAULT_TEST_TIMEOUT * 3)) {
	printf("Text before sleeping.\n");
	my_sleep(PCUT_DEFAULT_TEST_TIMEOUT * 2);
	printf("Text after the sleep.\n");
}

PCUT_MAIN()
