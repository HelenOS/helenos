/*
 * SPDX-FileCopyrightText: 2000 The NetBSD Foundation, Inc.
 * SPDX-FileCopyrightText: 2008 Tim Post
 *
 * SPDX-License-Identifier: BSD-3-Clause AND LicenseRef-GetOptLicense
 */

/* $NetBSD: getopt.h,v 1.10.6.1 2008/05/18 12:30:09 yamt Exp $ */

/* Ported to HelenOS August 2008 by Tim Post <echo@echoreply.us> */

#ifndef _GETOPT_H_
#define _GETOPT_H_

/*
 * Gnu like getopt_long() and BSD4.4 getsubopt()/optreset extensions
 */
#define no_argument        0
#define required_argument  1
#define optional_argument  2

struct option {
	/* name of long option */
	const char *name;
	/*
	 * one of no_argument, required_argument, and optional_argument:
	 * whether option takes an argument
	 */
	int has_arg;
	/* if not NULL, set *flag to val when option found */
	int *flag;
	/* if flag not NULL, value to set *flag to; else return value */
	int val;
};

/* HelenOS Port - These need to be exposed for legacy getopt() */
extern char *optarg;
extern int optind, opterr, optopt;
extern int optreset;

int getopt_long(int, char *const *, const char *,
    const struct option *, int *);

/* HelenOS Port : Expose legacy getopt() */
int	 getopt(int, char *const [], const char *);

#endif /* !_GETOPT_H_ */
