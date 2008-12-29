/*
 * Copyright (c) 2007 Martin Decky
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

/** @addtogroup tester
 * @{
 */
/** @file
 */

#ifndef TESTER_H_
#define TESTER_H_

#include <sys/types.h>
#include <bool.h>
#include <ipc/ipc.h>

#define IPC_TEST_START	10000
#define MAX_PHONES		20
#define MAX_CONNECTIONS 50
#define TEST_SKIPPED    "Test Skipped"

extern int myservice;
extern int phones[MAX_PHONES];
extern int connections[MAX_CONNECTIONS];
extern ipc_callid_t callids[MAX_CONNECTIONS];

typedef char * (* test_entry_t)(bool);

typedef struct {
	char * name;
	char * desc;
	test_entry_t entry;
	bool safe;
} test_t;

extern char * test_thread1(bool quiet);
extern char * test_print1(bool quiet);
extern char * test_fault1(bool quiet);
extern char * test_fault2(bool quiet);
extern char * test_register(bool quiet);
extern char * test_connect(bool quiet);
extern char * test_send_async(bool quiet);
extern char * test_send_sync(bool quiet);
extern char * test_answer(bool quiet);
extern char * test_hangup(bool quiet);
extern char * test_devmap1(bool quiet);
extern char * test_loop1(bool quiet);
extern char * test_vfs1(bool quiet);
extern char * test_console1(bool quiet);
extern char * test_stdio1(bool quiet);

extern test_t tests[];

#endif

/** @}
 */
