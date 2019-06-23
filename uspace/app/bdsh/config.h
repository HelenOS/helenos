/*
 * Copyright (c) 2008 Tim Post
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

/*
 * Various things that are used in many places including a few
 * tidbits left over from autoconf prior to the HelenOS port
 */

/* Specific port work-arounds : */
#ifndef PATH_MAX
#define PATH_MAX 255
#endif

#ifndef EXIT_SUCCESS
#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#endif

/* define maximal nested aliases */
#ifndef HUBS_MAX
#define HUBS_MAX 20
#endif

/* Used in many places */
#define SMALL_BUFLEN 256
#define LARGE_BUFLEN 1024

/*
 * How many words (arguments) are permitted, how big can a whole
 * sentence be? Similar to ARG_MAX
 */
#define WORD_MAX 1023
#define INPUT_MAX 4096

/* Leftovers from Autoconf */
#define PACKAGE_MAINTAINER "Tim Post"
#define PACKAGE_BUGREPORT "echo@echoreply.us"
#define PACKAGE_NAME "bdsh"
#define PACKAGE_STRING "The brain dead shell"
#define PACKAGE_TARNAME "bdsh"
#define PACKAGE_VERSION "0.1.0"
