/*
 * SPDX-FileCopyrightText: 2009 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
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

static const char *copyright = STRING(HELENOS_COPYRIGHT);
static const char *release = STRING(HELENOS_RELEASE);
static const char *name = STRING(HELENOS_CODENAME);
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
