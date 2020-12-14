/*	$NetBSD: getopt_long.c,v 1.21.4.1 2008/01/09 01:34:14 matt Exp $	*/

/*
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Dieter Baron and Thomas Klausner.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/* Ported to HelenOS August 2008 by Tim Post <echo@echoreply.us> */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <getopt.h>
#include <str.h>

/*
 * HelenOS Port : We're incorporating only the modern getopt_long with wrappers
 * to keep legacy getopt() usage from breaking. All references to REPLACE_GETOPT
 * are dropped, we just include the code
 */

int	opterr = 1;		/* if error message should be printed */
int	optind = 1;		/* index into parent argv vector */
int	optopt = '?';		/* character checked for validity */
int	optreset;		/* reset getopt */
char	*optarg;		/* argument associated with option */

#define IGNORE_FIRST	(*options == '-' || *options == '+')
#define PRINT_ERROR	((opterr) && ((*options != ':') \
				      || (IGNORE_FIRST && options[1] != ':')))
/* HelenOS Port - POSIXLY_CORRECT is always false */
#define IS_POSIXLY_CORRECT 0
#define PERMUTE         (!IS_POSIXLY_CORRECT && !IGNORE_FIRST)
/* XXX: GNU ignores PC if *options == '-' */
#define IN_ORDER        (!IS_POSIXLY_CORRECT && *options == '-')

/* return values */
#define	BADCH	(int)'?'
#define	BADARG		((IGNORE_FIRST && options[1] == ':') \
			 || (*options == ':') ? (int)':' : (int)'?')
#define INORDER (int)1

static char EMSG[] = "";

static int getopt_internal(int, char **, const char *);
static int gcd(int, int);
static void permute_args(int, int, int, char **);

static char *place = EMSG; /* option letter processing */

/* XXX: set optreset to 1 rather than these two */
static int nonopt_start = -1; /* first non option argument (for permute) */
static int nonopt_end = -1;   /* first option after non options (for permute) */

/* Error messages */

/*
 * HelenOS Port: Calls to warnx() were eliminated (as we have no stderr that
 * may be redirected) and replaced with printf. As such, error messages now
 * end in a newline
 */

static const char recargchar[] = "option requires an argument -- %c\n";
static const char recargstring[] = "option requires an argument -- %s\n";
static const char ambig[] = "ambiguous option -- %.*s\n";
static const char noarg[] = "option doesn't take an argument -- %.*s\n";
static const char illoptchar[] = "unknown option -- %c\n";
static const char illoptstring[] = "unknown option -- %s\n";

/*
 * Compute the greatest common divisor of a and b.
 */
static int gcd(int a, int b)
{
	int c;

	c = a % b;
	while (c != 0) {
		a = b;
		b = c;
		c = a % b;
	}

	return b;
}

/*
 * Exchange the block from nonopt_start to nonopt_end with the block
 * from nonopt_end to opt_end (keeping the same order of arguments
 * in each block).
 */
static void permute_args(int panonopt_start, int panonopt_end, int opt_end,
    char **nargv)
{
	int cstart, cyclelen, i, j, ncycle, nnonopts, nopts, pos;
	char *swap;

	assert(nargv != NULL);

	/*
	 * compute lengths of blocks and number and size of cycles
	 */
	nnonopts = panonopt_end - panonopt_start;
	nopts = opt_end - panonopt_end;
	ncycle = gcd(nnonopts, nopts);
	cyclelen = (opt_end - panonopt_start) / ncycle;

	for (i = 0; i < ncycle; i++) {
		cstart = panonopt_end + i;
		pos = cstart;
		for (j = 0; j < cyclelen; j++) {
			if (pos >= panonopt_end)
				pos -= nnonopts;
			else
				pos += nopts;
			swap = nargv[pos];
			nargv[pos] = nargv[cstart];
			nargv[cstart] = swap;
		}
	}
}

/*
 * getopt_internal --
 *	Parse argc/argv argument vector.  Called by user level routines.
 *  Returns -2 if -- is found (can be long option or end of options marker).
 */
static int getopt_internal(int nargc, char **nargv, const char *options)
{
	const char *oli;				/* option letter list index */
	int optchar;

	assert(nargv != NULL);
	assert(options != NULL);

	optarg = NULL;

	/*
	 * XXX Some programs (like rsyncd) expect to be able to
	 * XXX re-initialize optind to 0 and have getopt_long(3)
	 * XXX properly function again.  Work around this braindamage.
	 */
	if (optind == 0)
		optind = 1;

	if (optreset)
		nonopt_start = nonopt_end = -1;
start:
	if (optreset || !*place) {		/* update scanning pointer */
		optreset = 0;
		if (optind >= nargc) {          /* end of argument vector */
			place = EMSG;
			if (nonopt_end != -1) {
				/* do permutation, if we have to */
				permute_args(nonopt_start, nonopt_end,
				    optind, nargv);
				optind -= nonopt_end - nonopt_start;
			} else if (nonopt_start != -1) {
				/*
				 * If we skipped non-options, set optind
				 * to the first of them.
				 */
				optind = nonopt_start;
			}
			nonopt_start = nonopt_end = -1;
			return -1;
		}
		if ((*(place = nargv[optind]) != '-') ||
		    (place[1] == '\0')) {    /* found non-option */
			place = EMSG;
			if (IN_ORDER) {
				/*
				 * GNU extension:
				 * return non-option as argument to option 1
				 */
				optarg = nargv[optind++];
				return INORDER;
			}
			if (!PERMUTE) {
				/*
				 * if no permutation wanted, stop parsing
				 * at first non-option
				 */
				return -1;
			}
			/* do permutation */
			if (nonopt_start == -1)
				nonopt_start = optind;
			else if (nonopt_end != -1) {
				permute_args(nonopt_start, nonopt_end,
				    optind, nargv);
				nonopt_start = optind -
				    (nonopt_end - nonopt_start);
				nonopt_end = -1;
			}
			optind++;
			/* process next argument */
			goto start;
		}
		if (nonopt_start != -1 && nonopt_end == -1)
			nonopt_end = optind;
		if (place[1] && *++place == '-') {	/* found "--" */
			place++;
			return -2;
		}
	}
	if ((optchar = (int)*place++) == (int)':' ||
	    (oli = str_chr(options + (IGNORE_FIRST ? 1 : 0), optchar)) == NULL) {
		/* option letter unknown or ':' */
		if (!*place)
			++optind;
		if (PRINT_ERROR)
			printf(illoptchar, optchar);
		optopt = optchar;
		return BADCH;
	}
	if (optchar == 'W' && oli[1] == ';') {		/* -W long-option */
		/* XXX: what if no long options provided (called by getopt)? */
		if (*place)
			return -2;

		if (++optind >= nargc) {	/* no arg */
			place = EMSG;
			if (PRINT_ERROR)
				printf(recargchar, optchar);
			optopt = optchar;
			return BADARG;
		} else				/* white space */
			place = nargv[optind];
		/*
		 * Handle -W arg the same as --arg (which causes getopt to
		 * stop parsing).
		 */
		return -2;
	}
	if (*++oli != ':') {			/* doesn't take argument */
		if (!*place)
			++optind;
	} else {				/* takes (optional) argument */
		optarg = NULL;
		if (*place)			/* no white space */
			optarg = place;
		/* XXX: disable test for :: if PC? (GNU doesn't) */
		else if (oli[1] != ':') {	/* arg not optional */
			if (++optind >= nargc) {	/* no arg */
				place = EMSG;
				if (PRINT_ERROR)
					printf(recargchar, optchar);
				optopt = optchar;
				return BADARG;
			} else
				optarg = nargv[optind];
		}
		place = EMSG;
		++optind;
	}
	/* dump back option letter */
	return optchar;
}

/*
 * getopt --
 *	Parse argc/argv argument vector.
 */
int getopt(int nargc, char *const *nargv, const char *options)
{
	int retval;

	assert(nargv != NULL);
	assert(options != NULL);

	retval = getopt_internal(nargc, (char **)nargv, options);
	if (retval == -2) {
		++optind;
		/*
		 * We found an option (--), so if we skipped non-options,
		 * we have to permute.
		 */
		if (nonopt_end != -1) {
			permute_args(nonopt_start, nonopt_end, optind,
			    (char **)nargv);
			optind -= nonopt_end - nonopt_start;
		}
		nonopt_start = nonopt_end = -1;
		retval = -1;
	}
	return retval;
}

/*
 * getopt_long --
 *	Parse argc/argv argument vector.
 */
int getopt_long(int nargc, char *const *nargv, const char *options,
    const struct option *long_options, int *idx)
{
	int retval;

#define IDENTICAL_INTERPRETATION(_x, _y)				\
	(long_options[(_x)].has_arg == long_options[(_y)].has_arg &&	\
	 long_options[(_x)].flag == long_options[(_y)].flag &&		\
	 long_options[(_x)].val == long_options[(_y)].val)

	assert(nargv != NULL);
	assert(options != NULL);
	assert(long_options != NULL);
	/* idx may be NULL */

	retval = getopt_internal(nargc, (char **)nargv, options);
	if (retval == -2) {
		char *current_argv;
		char *has_equal;
		size_t current_argv_len;
		int i, ambiguous, match;

		current_argv = (char *)place;
		match = -1;
		ambiguous = 0;

		optind++;
		place = EMSG;

		if (*current_argv == '\0') {		/* found "--" */
			/*
			 * We found an option (--), so if we skipped
			 * non-options, we have to permute.
			 */
			if (nonopt_end != -1) {
				permute_args(nonopt_start, nonopt_end,
				    optind, (char **)nargv);
				optind -= nonopt_end - nonopt_start;
			}
			nonopt_start = nonopt_end = -1;
			return -1;
		}
		if ((has_equal = str_chr(current_argv, '=')) != NULL) {
			/* argument found (--option=arg) */
			current_argv_len = has_equal - current_argv;
			has_equal++;
		} else
			current_argv_len = str_size(current_argv);

		for (i = 0; long_options[i].name; i++) {
			/* find matching long option */
			if (str_lcmp(current_argv, long_options[i].name,
			    str_nlength(current_argv, current_argv_len)))
				continue;

			if (str_size(long_options[i].name) ==
			    (unsigned)current_argv_len) {
				/* exact match */
				match = i;
				ambiguous = 0;
				break;
			}
			if (match == -1)		/* partial match */
				match = i;
			else if (!IDENTICAL_INTERPRETATION(i, match))
				ambiguous = 1;
		}
		if (ambiguous) {
			/* ambiguous abbreviation */
			if (PRINT_ERROR)
				printf(ambig, (int)current_argv_len,
				    current_argv);
			optopt = 0;
			return BADCH;
		}
		if (match != -1) {			/* option found */
			if (long_options[match].has_arg == no_argument &&
			    has_equal) {
				if (PRINT_ERROR)
					printf(noarg, (int)current_argv_len,
					    current_argv);
				/*
				 * XXX: GNU sets optopt to val regardless of
				 * flag
				 */
				if (long_options[match].flag == NULL)
					optopt = long_options[match].val;
				else
					optopt = 0;
				return BADARG;
			}
			if (long_options[match].has_arg == required_argument ||
			    long_options[match].has_arg == optional_argument) {
				if (has_equal)
					optarg = has_equal;
				else if (long_options[match].has_arg ==
				    required_argument) {
					/*
					 * optional argument doesn't use
					 * next nargv
					 */
					optarg = nargv[optind++];
				}
			}
			if ((long_options[match].has_arg == required_argument) &&
			    (optarg == NULL)) {
				/*
				 * Missing argument; leading ':'
				 * indicates no error should be generated
				 */
				if (PRINT_ERROR)
					printf(recargstring, current_argv);
				/*
				 * XXX: GNU sets optopt to val regardless
				 * of flag
				 */
				if (long_options[match].flag == NULL)
					optopt = long_options[match].val;
				else
					optopt = 0;
				--optind;
				return BADARG;
			}
		} else {			/* unknown option */
			if (PRINT_ERROR)
				printf(illoptstring, current_argv);
			optopt = 0;
			return BADCH;
		}
		if (long_options[match].flag) {
			*long_options[match].flag = long_options[match].val;
			retval = 0;
		} else
			retval = long_options[match].val;
		if (idx)
			*idx = match;
	}
	return retval;
#undef IDENTICAL_INTERPRETATION
}
