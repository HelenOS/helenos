/*
 * SPDX-FileCopyrightText: 2016 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libcamd64
 * @brief
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <stdlib.h>

#include <rtld/elf_dyn.h>
#include <rtld/dynamic.h>

void dyn_parse_arch(elf_dyn_t *dp, size_t bias, dyn_info_t *info)
{
	(void) dp;
	(void) bias;
	(void) info;
}

/** @}
 */
