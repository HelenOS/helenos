/*
 * Copyright (c) 2013 Jiri Svoboda
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

/** @addtogroup corecfg
 * @{
 */
/** @file Core file configuration utility.
 */

#include <corecfg.h>
#include <errno.h>
#include <stdio.h>
#include <str.h>

#define NAME "corecfg"

static void print_syntax(void)
{
	printf("Syntax:\n");
	printf("\t%s [get]\n", NAME);
	printf("\t%s enable\n", NAME);
	printf("\t%s disable\n", NAME);
}

static int corecfg_print(void)
{
	bool enable;
	errno_t rc;

	rc = corecfg_get_enable(&enable);
	if (rc != EOK) {
		printf("Failed getting core file setting.\n");
		return 1;
	}

	printf("Core files: %s.\n", enable ? "enabled" : "disabled");

	return 0;
}

int main(int argc, char *argv[])
{
	errno_t rc;

	rc = corecfg_init();
	if (rc != EOK) {
		printf("Failed contacting corecfg service.\n");
		return 1;
	}

	if ((argc < 2) || (str_cmp(argv[1], "get") == 0))
		return corecfg_print();
	else if (str_cmp(argv[1], "enable") == 0)
		return corecfg_set_enable(true);
	else if (str_cmp(argv[1], "disable") == 0)
		return corecfg_set_enable(false);
	else {
		printf("%s: Unknown command '%s'.\n", NAME, argv[1]);
		print_syntax();
		return 1;
	}

	return 0;
}

/** @}
 */
