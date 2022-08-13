/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <version.h>
#include <printf.h>
#include <macros.h>

static const char *project = "HelenOS bootloader";
static const char *copyright = STRING(HELENOS_COPYRIGHT);
static const char *release = STRING(HELENOS_RELEASE);
static const char *name = STRING(HELENOS_CODENAME);
static const char *arch = STRING(KARCH);

#ifdef REVISION
static const char *revision = ", revision " STRING(REVISION);
#else
static const char *revision = "";
#endif

#ifdef TIMESTAMP
static const char *timestamp = " on " STRING(TIMESTAMP);
#else
static const char *timestamp = "";
#endif

void version_print(void)
{
	printf("%s, release %s (%s)%s\nBuilt%s for %s\n%s\n",
	    project, release, name, revision, timestamp, arch, copyright);
}
