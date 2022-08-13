/*
 * SPDX-FileCopyrightText: 2017 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_genarch
 * @{
 */
/** @file
 */

#include <genarch/multiboot/multiboot.h>
#include <config.h>
#include <stddef.h>

void multiboot_cmdline(const char *cmdline)
{
	/*
	 * GRUB passes the command line in an escaped form.
	 */
	for (size_t i = 0, j = 0;
	    cmdline[i] && j < CONFIG_BOOT_ARGUMENTS_BUFLEN;
	    i++, j++) {
		if (cmdline[i] == '\\') {
			switch (cmdline[i + 1]) {
			case '\\':
			case '\'':
			case '\"':
				i++;
				break;
			}
		}
		bargs[j] = cmdline[i];
	}
}

/** @}
 */
