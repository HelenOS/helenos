/*
 * SPDX-FileCopyrightText: 2010 Martin Decky
 * SPDX-FileCopyrightText: 2017 CZ.NIC, z.s.p.o.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libc
 * @{
 */
/** @file
 */

#include <errno.h>
#include <str_error.h>
#include <stdio.h>
#include <fibril.h>

#define NOERR_LEN  64

/*
 * The arrays below are automatically generated from the same file that
 * errno.h constants are generated from. Triple-include of the same list
 * with redefinitions of __errno() macro are used to ensure that the
 * information cannot get out of synch. This is inpired by musl libc.
 */

#undef __errno_entry
#define __errno_entry(name, num, desc) name,

static const errno_t err_num[] = {
#include <abi/errno.in>
};

#undef __errno_entry
#define __errno_entry(name, num, desc) #name,

static const char *err_name[] = {
#include <abi/errno.in>
};

#undef __errno_entry
#define __errno_entry(name, num, desc) "[" #name "] " desc,

static const char *err_desc[] = {
#include <abi/errno.in>
};

static fibril_local char noerr[NOERR_LEN];

/* Returns index corresponding to the given errno, or -1 if not found. */
static int find_errno(errno_t e)
{
	/*
	 * Just a dumb linear search.
	 * There are too few entries to warrant anything smarter.
	 */

	int len = sizeof(err_num) / sizeof(errno_t);

	for (int i = 0; i < len; i++) {
		if (err_num[i] == e) {
			return i;
		}
	}

	return -1;
}

const char *str_error_name(errno_t e)
{
	int i = find_errno(e);

	if (i >= 0) {
		return err_name[i];
	}

	snprintf(noerr, NOERR_LEN, "(%d)", (int)e);
	return noerr;
}

const char *str_error(errno_t e)
{
	int i = find_errno(e);

	if (i >= 0) {
		return err_desc[i];
	}

	snprintf(noerr, NOERR_LEN, "Unknown error code (%d)", (int)e);
	return noerr;
}

/** @}
 */
