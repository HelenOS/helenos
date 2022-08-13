/*
 * SPDX-FileCopyrightText: 2006 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_generic
 * @{
 */
/** @file
 */

#include <main/version.h>
#include <stdio.h>
#include <macros.h>

static const char *project = "SPARTAN kernel";
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

/** Print version information. */
void version_print(void)
{
	printf("%s, release %s (%s)%s\nBuilt%s for %s\n%s\n",
	    project, release, name, revision, timestamp, arch, copyright);
}

/** @}
 */
