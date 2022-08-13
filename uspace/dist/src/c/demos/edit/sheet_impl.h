/*
 * SPDX-FileCopyrightText: 2012 Jiri Svoboda
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup edit
 * @{
 */
/**
 * @file
 */

#ifndef SHEET_IMPL_H__
#define SHEET_IMPL_H__

#include "sheet.h"

/** Sheet */
struct sheet {
	/* Note: This structure is opaque for the user. */

	size_t text_size;
	size_t dbuf_size;
	char *data;

	list_t tags;
};

#endif

/** @}
 */
