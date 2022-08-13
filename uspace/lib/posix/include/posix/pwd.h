/*
 * SPDX-FileCopyrightText: 2011 Jiri Zarevucky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup libposix
 * @{
 */
/** @file Password handling.
 */

#ifndef POSIX_PWD_H_
#define POSIX_PWD_H_

#include <sys/types.h>
#include <_bits/decls.h>

__C_DECLS_BEGIN;

struct passwd {
	char *pw_name;
	uid_t pw_uid;
	gid_t pw_gid;
	char *pw_dir;
	char *pw_shell;
};

extern struct passwd *getpwent(void);
extern void setpwent(void);
extern void endpwent(void);

extern struct passwd *getpwnam(const char *name);
extern int getpwnam_r(const char *name, struct passwd *pwd,
    char *buffer, size_t bufsize, struct passwd **result);

extern struct passwd *getpwuid(uid_t uid);
extern int getpwuid_r(uid_t uid, struct passwd *pwd,
    char *buffer, size_t bufsize, struct passwd **result);

__C_DECLS_END;

#endif /* POSIX_PWD_H_ */

/** @}
 */
