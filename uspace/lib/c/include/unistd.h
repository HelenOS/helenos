/*
 * Copyright (c) 2005 Martin Decky
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

/** @addtogroup libc
 * @{
 */
/** @file
 */

#ifndef LIBC_UNISTD_H_
#define LIBC_UNISTD_H_

#include <sys/types.h>
#include <time.h>
#include <libarch/config.h>

#ifndef NULL
	#define NULL  ((void *) 0)
#endif

#ifndef SEEK_SET
	#define SEEK_SET  0
#endif

#ifndef SEEK_CUR
	#define SEEK_CUR  1
#endif

#ifndef SEEK_END
	#define SEEK_END  2
#endif

#define getpagesize()  (PAGE_SIZE)

extern int dup2(int, int);

extern ssize_t write(int, const void *, size_t);
extern ssize_t read(int, void *, size_t);

extern ssize_t read_all(int, void *, size_t);
extern ssize_t write_all(int, const void *, size_t);

extern off64_t lseek(int, off64_t, int);
extern int ftruncate(int, aoff64_t);

extern int close(int);
extern int fsync(int);
extern int unlink(const char *);

extern char *getcwd(char *, size_t);
extern int rmdir(const char *);
extern int chdir(const char *);

extern void exit(int) __attribute__((noreturn));
extern int usleep(useconds_t);
extern unsigned int sleep(unsigned int);

#endif

/** @}
 */
