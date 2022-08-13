/*
 * SPDX-FileCopyrightText: 2005 Ondrej Palkovsky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_amd64_mm
 * @{
 */
/** @file
 */

#ifndef KERN_amd64_PTL_H_
#define KERN_amd64_PTL_H_

#define PTL_NO_EXEC        (1 << 63)
#define PTL_ACCESSED       (1 << 5)
#define PTL_CACHE_DISABLE  (1 << 4)
#define PTL_CACHE_THROUGH  (1 << 3)
#define PTL_USER           (1 << 2)
#define PTL_WRITABLE       (1 << 1)
#define PTL_PRESENT        1
#define PTL_2MB_PAGE       (1 << 7)

#endif

/** @}
 */
