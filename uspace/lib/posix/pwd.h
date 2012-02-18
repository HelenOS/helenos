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

#ifndef POSIX_PWD_H_
#define POSIX_PWD_H_

#include "sys/types.h"

struct posix_passwd {
	char *pw_name;
	posix_uid_t pw_uid;
	posix_gid_t pw_gid;
	char *pw_dir;
	char *pw_shell;
};

extern struct posix_passwd *posix_getpwent(void);
extern void posix_setpwent(void);
extern void posix_endpwent(void);

extern struct posix_passwd *posix_getpwnam(const char *name);
extern int posix_getpwnam_r(const char *name, struct posix_passwd *pwd,
    char *buffer, size_t bufsize, struct posix_passwd **result);

extern struct posix_passwd *posix_getpwuid(posix_uid_t uid);
extern int posix_getpwuid_r(posix_uid_t uid, struct posix_passwd *pwd,
    char *buffer, size_t bufsize, struct posix_passwd **result);

#ifndef LIBPOSIX_INTERNAL
	#define passwd posix_passwd
	
	#define getpwent posix_getpwent
	#define setpwent posix_setpwent
	#define endpwent posix_endpwent

	#define getpwnam posix_getpwnam
	#define getpwnam_r posix_getpwnam_r

	#define getpwuid posix_getpwuid
	#define getpwuid_r posix_getpwuid_r
#endif

#endif /* POSIX_PWD_H_ */

/** @}
 */
