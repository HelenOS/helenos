/*
 * SPDX-FileCopyrightText: 2013 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
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
