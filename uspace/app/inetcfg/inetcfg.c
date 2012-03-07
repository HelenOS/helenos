/*
 * Copyright (c) 2012 Jiri Svoboda
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

/** @addtogroup inetcfg
 * @{
 */
/** @file Internet configuration utility.
 *
 * Controls the internet service (@c inet).
 */

#include <async.h>
#include <errno.h>
#include <inet/inetcfg.h>
#include <stdio.h>
#include <str_error.h>
#include <sys/types.h>

#define NAME "inetcfg"

static void print_syntax(void)
{
	printf("syntax: " NAME " xxx\n");
}

int main(int argc, char *argv[])
{
	int rc;
	inet_naddr_t naddr;
	sysarg_t addr_id;

	if (argc > 1) {
		printf(NAME ": Invalid argument '%s'.\n", argv[1]);
		print_syntax();
		return 1;
	}

	printf("initialize\n");

	rc = inetcfg_init();
	if (rc != EOK) {
		printf(NAME ": Failed connecting to internet service (%d).\n",
		    rc);
		return 1;
	}

	printf("sleep\n");
	async_usleep(10*1000*1000);
	printf("create static addr\n");

	rc = inetcfg_addr_create_static("v4s", &naddr, &addr_id);
	if (rc != EOK) {
		printf(NAME ": Failed creating static address '%s' (%d)\n",
		    "v4s", rc);
		return 1;
	}

	printf("Success!\n");
	return 0;
}

/** @}
 */
