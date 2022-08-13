/*
 * SPDX-FileCopyrightText: 2012 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file
 */

#ifndef BOOT_PRINTF_VERIFY_H_
#define BOOT_PRINTF_VERIFY_H_

#ifdef __clang__
#define _HELENOS_PRINTF_ATTRIBUTE(start, end) \
	__attribute__((format(__printf__, start, end)))
#else
#define _HELENOS_PRINTF_ATTRIBUTE(start, end) \
	__attribute__((format(gnu_printf, start, end)))
#endif

#endif

/** @}
 */
