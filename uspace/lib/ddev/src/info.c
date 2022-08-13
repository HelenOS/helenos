/*
 * SPDX-FileCopyrightText: 2020 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <ddev/info.h>
#include <mem.h>

/** Initialize display device information structure.
 *
 * @param info Display device information structure
 */
void ddev_info_init(ddev_info_t *info)
{
	memset(info, 0, sizeof(ddev_info_t));
}

/** @}
 */
