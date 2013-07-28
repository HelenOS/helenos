/*
 * Copyright (c) 2011 Jiri Zarevucky
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
/** @file Password handling.
 */

#define LIBPOSIX_INTERNAL
#define __POSIX_DEF__(x) posix_##x

#include "libc/stdbool.h"
#include "posix/pwd.h"
#include "posix/string.h"
#include "posix/errno.h"
#include "posix/assert.h"

static bool entry_read = false;

/* dummy user account */
static const struct posix_passwd dummy_pwd = {
	.pw_name = (char *) "user",
	.pw_uid = 0,
	.pw_gid = 0,
	.pw_dir = (char *) "/",
	.pw_shell = (char *) "/app/bdsh"
};

/**
 * Retrieve next broken-down entry from the user database.
 *
 * Since HelenOS doesn't have user accounts, this always returns
 * the same made-up entry.
 *
 * @return Next user database entry or NULL if not possible. Since HelenOS
 *     doesn't have user accounts, this always returns the same made-up entry.
 */
struct posix_passwd *posix_getpwent(void)
{
	if (entry_read) {
		return NULL;
	}

	entry_read = true;
	return (struct posix_passwd *) &dummy_pwd;
}

/**
 * Rewind the user list.
 */
void posix_setpwent(void)
{
	entry_read = false;
}

/**
 * Ends enumerating and releases all resources. (Noop)
 */
void posix_endpwent(void)
{
	/* noop */
}

/**
 * Find an entry by name.
 *
 * @param name Name of the entry.
 * @return Either found entry or NULL if no such entry exists.
 */
struct posix_passwd *posix_getpwnam(const char *name)
{
	assert(name != NULL);

	if (posix_strcmp(name, "user") != 0) {
		return NULL;
	}

	return (struct posix_passwd *) &dummy_pwd;
}

/**
 * Find an entry by name, thread safely.
 *
 * @param name Name of the entry.
 * @param pwd Original structure.
 * @param buffer Buffer for the strings referenced from the result structure.
 * @param bufsize Length of the buffer.
 * @param result Where to store updated structure.
 * @return Zero on success (either found or not found, but without an error),
 *     non-zero error number if error occured.
 */
int posix_getpwnam_r(const char *name, struct posix_passwd *pwd,
    char *buffer, size_t bufsize, struct posix_passwd **result)
{
	assert(name != NULL);
	assert(pwd != NULL);
	assert(buffer != NULL);
	assert(result != NULL);
	
	if (posix_strcmp(name, "user") != 0) {
		*result = NULL;
		return 0;
	}
	
	return posix_getpwuid_r(0, pwd, buffer, bufsize, result);
}

/**
 * Find an entry by UID.
 *
 * @param uid UID of the entry.
 * @return Either found entry or NULL if no such entry exists.
 */
struct posix_passwd *posix_getpwuid(posix_uid_t uid)
{
	if (uid != 0) {
		return NULL;
	}

	return (struct posix_passwd *) &dummy_pwd;
}

/**
 * Find an entry by UID, thread safely.
 *
 * @param uid UID of the entry.
 * @param pwd Original structure.
 * @param buffer Buffer for the strings referenced from the result structure.
 * @param bufsize Length of the buffer.
 * @param result Where to store updated structure.
 * @return Zero on success (either found or not found, but without an error),
 *     non-zero error number if error occured.
 */
int posix_getpwuid_r(posix_uid_t uid, struct posix_passwd *pwd,
    char *buffer, size_t bufsize, struct posix_passwd **result)
{
	assert(pwd != NULL);
	assert(buffer != NULL);
	assert(result != NULL);
	
	static const char bf[] = { 'u', 's', 'e', 'r', '\0',
	    '/', '\0', 'b', 'd', 's', 'h', '\0' };
	
	if (uid != 0) {
		*result = NULL;
		return 0;
	}
	if (bufsize < sizeof(bf)) {
		*result = NULL;
		return ERANGE;
	}

	memcpy(buffer, bf, sizeof(bf));

	pwd->pw_name = (char *) bf;
	pwd->pw_uid = 0;
	pwd->pw_gid = 0;
	pwd->pw_dir = (char *) bf + 5;
	pwd->pw_shell = (char *) bf + 7;
	*result = (struct posix_passwd *) pwd;

	return 0;
}

/** @}
 */
