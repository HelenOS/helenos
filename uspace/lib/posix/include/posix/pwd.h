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

#ifndef __POSIX_DEF__
#define __POSIX_DEF__(x) x
#endif

#include "sys/types.h"

struct __POSIX_DEF__(passwd) {
	char *pw_name;
	__POSIX_DEF__(uid_t) pw_uid;
	__POSIX_DEF__(gid_t) pw_gid;
	char *pw_dir;
	char *pw_shell;
};

extern struct __POSIX_DEF__(passwd) *__POSIX_DEF__(getpwent)(void);
extern void __POSIX_DEF__(setpwent)(void);
extern void __POSIX_DEF__(endpwent)(void);

extern struct __POSIX_DEF__(passwd) *__POSIX_DEF__(getpwnam)(const char *name);
extern int __POSIX_DEF__(getpwnam_r)(const char *name, struct __POSIX_DEF__(passwd) *pwd,
    char *buffer, size_t bufsize, struct __POSIX_DEF__(passwd) **result);

extern struct __POSIX_DEF__(passwd) *__POSIX_DEF__(getpwuid)(__POSIX_DEF__(uid_t) uid);
extern int __POSIX_DEF__(getpwuid_r)(__POSIX_DEF__(uid_t) uid, struct __POSIX_DEF__(passwd) *pwd,
    char *buffer, size_t bufsize, struct __POSIX_DEF__(passwd) **result);


#endif /* POSIX_PWD_H_ */

/** @}
 */
