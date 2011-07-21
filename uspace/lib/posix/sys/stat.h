/*
 * Copyright (c) 2011 Jiri Zarevucky
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
/** @file File status handling.
 */

#ifndef POSIX_SYS_STAT_H_
#define POSIX_SYS_STAT_H_

#include "../libc/sys/stat.h"
#include "types.h"
#include "../time.h"

/* values are the same as on Linux */

#undef S_IFMT
#undef S_IFSOCK
#undef S_IFLNK
#undef S_IFREG
#undef S_IFBLK
#undef S_IFDIR
#undef S_IFCHR
#undef S_IFIFO
#define S_IFMT     0170000   /* all file types */
#define S_IFSOCK   0140000   /* socket */
#define S_IFLNK    0120000   /* symbolic link */
#define S_IFREG    0100000   /* regular file */
#define S_IFBLK    0060000   /* block device */
#define S_IFDIR    0040000   /* directory */
#define S_IFCHR    0020000   /* character device */
#define S_IFIFO    0010000   /* FIFO */

#undef S_ISUID
#undef S_ISGID
#undef S_ISVTX
#define S_ISUID    0004000   /* SUID */
#define S_ISGID    0002000   /* SGID */
#define S_ISVTX    0001000   /* sticky */

#undef S_IRWXU
#undef S_IRUSR
#undef S_IWUSR
#undef S_IXUSR
#define S_IRWXU    00700     /* owner permissions */
#define S_IRUSR    00400
#define S_IWUSR    00200
#define S_IXUSR    00100

#undef S_IRWXG
#undef S_IRGRP
#undef S_IWGRP
#undef S_IXGRP
#define S_IRWXG    00070     /* group permissions */
#define S_IRGRP    00040
#define S_IWGRP    00020
#define S_IXGRP    00010

#undef S_IRWXO
#undef S_IROTH
#undef S_IWOTH
#undef S_IXOTH
#define S_IRWXO    00007     /* other permissions */
#define S_IROTH    00004
#define S_IWOTH    00002
#define S_IXOTH    00001

#undef S_ISREG
#undef S_ISDIR
#undef S_ISCHR
#undef S_ISBLK
#undef S_ISFIFO
#undef S_ISLNK
#undef S_ISSOCK
#define S_ISREG(m) ((m & S_IFREG) != 0)
#define S_ISDIR(m) ((m & S_IFDIR) != 0)
#define S_ISCHR(m) ((m & S_IFCHR) != 0)
#define S_ISBLK(m) ((m & S_IFBLK) != 0)
#define S_ISFIFO(m) ((m & S_IFIFO) != 0)
#define S_ISLNK(m) ((m & S_IFLNK) != 0) /* symbolic link? (Not in POSIX.1-1996.) */
#define S_ISSOCK(m) ((m & S_IFSOCK) != 0) /* socket? (Not in POSIX.1-1996.) */

struct posix_stat {
	posix_dev_t     st_dev;     /* ID of device containing file */
	posix_ino_t     st_ino;     /* inode number */
	mode_t          st_mode;    /* protection */
	posix_nlink_t   st_nlink;   /* number of hard links */
	posix_uid_t     st_uid;     /* user ID of owner */
	posix_gid_t     st_gid;     /* group ID of owner */
	posix_dev_t     st_rdev;    /* device ID (if special file) */
	posix_off_t     st_size;    /* total size, in bytes */
	posix_blksize_t st_blksize; /* blocksize for file system I/O */
	posix_blkcnt_t  st_blocks;  /* number of 512B blocks allocated */
	time_t          st_atime;   /* time of last access */
	time_t          st_mtime;   /* time of last modification */
	time_t          st_ctime;   /* time of last status change */
};

extern int posix_fstat(int fd, struct posix_stat *st);
extern int posix_lstat(const char *restrict path, struct posix_stat *restrict st);
extern int posix_stat(const char *restrict path, struct posix_stat *restrict st);
extern int posix_chmod(const char *path, mode_t mode);
extern mode_t posix_umask(mode_t mask);

#ifndef LIBPOSIX_INTERNAL
	#define fstat posix_fstat
	#define lstat posix_lstat
	#define stat posix_stat
	#define chmod posix_chmod
	#define umask posix_umask
#endif

#endif /* POSIX_SYS_STAT_H */

/** @}
 */
