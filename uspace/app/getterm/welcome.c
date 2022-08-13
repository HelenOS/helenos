/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup getterm
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include "welcome.h"

/** Welcome message and survival tips. */
void welcome_msg_print(void)
{
	printf("Welcome to HelenOS!\n");
	printf("http://www.helenos.org/\n\n");
	printf("Type 'help' [Enter] to see a few survival tips.\n\n");
}

/** @}
 */
