/*
 * SPDX-FileCopyrightText: 2008 Tim Post
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ERRSTR_H
#define ERRSTR_H

/* Simple array to translate error codes to meaningful strings */

static const char *cl_errors[] = {
	"Success",
	"Failure",
	"Busy",
	"No Such Entry",
	"Not Enough Memory",
	"Permission Denied",
	"Method Not Supported",
	"Bad command or file name",
	"Entry already exists",
	"Object too large",
	NULL
};

static const char *err2str(int);

#endif
