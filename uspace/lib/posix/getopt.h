/*
 * Copyright (c) 2011 Petr Koupy
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup libposix
 * @{
 */
/** @file
 */

#ifndef POSIX_GETOPT_H_
#define POSIX_GETOPT_H_

#undef no_argument
#undef required_argument
#undef optional_argument
#define no_argument        0
#define required_argument  1
#define optional_argument  2

struct posix_option {
	const char *name; /* name of the option */
	int has_arg; /* no_argument / required_argument / optional_argument */
	int *flag; /* TODO */
	int val; /* TODO */
};

extern int posix_getopt_long(int argc, char * const argv[],
    const char *optstring, const struct posix_option *longopts, int *longindex);

extern int posix_getopt_long_only(int argc, char * const argv[],
    const char *optstring, const struct posix_option *longopts, int *longindex);

#ifndef LIBPOSIX_INTERNAL
	#define option posix_option
	#define posix_getopt_long posix_getopt_long
	#define getopt_long_only posix_getopt_long_only
#endif

#endif /* POSIX_GETOPT_H_ */

/** @}
 */
