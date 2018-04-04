/*
 * Copyright (c) 2009 Martin Decky
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

/** @addtogroup getterm
 * @{
 */
/**
 * @file
 */

#include <stdio.h>
#include <macros.h>
#include "getterm.h"
#include "version.h"

static const char *copyright = STRING(COPYRIGHT);
static const char *release = STRING(RELEASE);
static const char *name = STRING(NAME);
static const char *arch = STRING(UARCH);

#ifdef REVISION
static const char *revision = ", revision " STRING(REVISION);
#else
static const char *revision = "";
#endif

#ifdef TIMESTAMP
static const char *timestamp = "\nBuilt on " STRING(TIMESTAMP);
#else
static const char *timestamp = "";
#endif

/** Print version information. */
void version_print(const char *term)
{
	printf("HelenOS release %s (%s)%s%s\n", release, name, revision, timestamp);
	printf("Running on %s (%s)\n", arch, term);
	printf("%s\n\n", copyright);
}

/** @}
 */
