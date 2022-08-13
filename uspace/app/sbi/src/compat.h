/*
 * SPDX-FileCopyrightText: 2010 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @file HelenOS compatibility hacks.
 *
 * This must be included later than HelenOS's list.h.
 * XXX A better way must be found than this.
 */

#ifndef COMPAT_H_
#define COMPAT_H_

#ifdef __HELENOS__

#define list sbi_list
#define list_t sbi_list_t

/*
 * Avoid name conflicts with ADT library.
 */
#define list_append sbi_list_append
#define list_prepend sbi_list_prepend
#define list_remove sbi_list_remove
#define list_first sbi_list_first
#define list_last sbi_list_last

#endif

#endif
