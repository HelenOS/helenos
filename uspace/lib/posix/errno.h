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
/** @file System error numbers.
 */

#ifndef POSIX_ERRNO_H_
#define POSIX_ERRNO_H_

#include "libc/errno.h"

/* IMPORTANT:
 * Since libc uses negative errorcodes, some sort of conversion is necessary to
 * keep POSIX programs and libraries from breaking. This file maps POSIX error
 * codes to absolute values of corresponding libc codes where available, and
 * assigns new code where there is no prior definition in libc.
 *
 * A new errno variable is defined. When accessed, the function first looks at
 * libc errno and iff it is != 0, sets the POSIX errno to absolute value of
 * libc errno. Given that no library function sets errno to 0 and that all
 * POSIX libraries will be used solely by POSIX programs (thus, there only needs
 * to be one way correspondence between errno and posix_errno), this approach
 * should work as expected in most cases and does not require any wrappers for
 * libc routines that would just change errno values.
 *
 * There is no conditioning by LIBPOSIX_INTERNAL for redefinitions of macros.
 * If there is a need to check errno for a value not defined by POSIX, it's
 * necessary to compare errno against abs(ECODE), because there is no
 * redefinition for such error codes.
 *
 * XXX: maybe all HOS error codes should be redefined
 *
 * NOTE: This redefinition is slightly POSIX incompatible, since the
 *  specification requires the macro values to be usable in preprocessing
 *  directives. I don't think that's very important, though.
 */

#undef errno
#define errno (*__posix_errno())

extern int *__posix_errno(void);

#define __TOP_ERRNO (-NO_DATA)

enum {
	POSIX_E2BIG = __TOP_ERRNO + 1,
	POSIX_EACCES = __TOP_ERRNO + 2,
	POSIX_EADDRINUSE = -EADDRINUSE,
	POSIX_EADDRNOTAVAIL = -EADDRNOTAVAIL,
	POSIX_EAFNOSUPPORT = -EAFNOSUPPORT,
	POSIX_EAGAIN = -EAGAIN,
	POSIX_EALREADY = __TOP_ERRNO + 3,
	POSIX_EBADF = -EBADF,
	POSIX_EBADMSG = __TOP_ERRNO + 4,
	POSIX_EBUSY = -EBUSY,
	POSIX_ECANCELED = __TOP_ERRNO + 5,
	POSIX_ECHILD = __TOP_ERRNO + 6,
	POSIX_ECONNABORTED = __TOP_ERRNO + 7,
	POSIX_ECONNREFUSED = __TOP_ERRNO + 8,
	POSIX_ECONNRESET = __TOP_ERRNO + 9,
	POSIX_EDEADLK = __TOP_ERRNO + 10,
	POSIX_EDESTADDRREQ = -EDESTADDRREQ,
	POSIX_EDOM = __TOP_ERRNO + 11,
	POSIX_EDQUOT = __TOP_ERRNO + 12,
	POSIX_EEXIST = -EEXIST,
	POSIX_EFAULT = __TOP_ERRNO + 13,
	POSIX_EFBIG = __TOP_ERRNO + 14,
	POSIX_EHOSTUNREACH = __TOP_ERRNO + 15,
	POSIX_EIDRM = __TOP_ERRNO + 16,
	POSIX_EILSEQ = __TOP_ERRNO + 17,
	POSIX_EINPROGRESS = -EINPROGRESS,
	POSIX_EINTR = -EINTR,
	POSIX_EINVAL = -EINVAL,
	POSIX_EIO = -EIO,
	POSIX_EISCONN = __TOP_ERRNO + 18,
	POSIX_EISDIR = -EISDIR,
	POSIX_ELOOP = __TOP_ERRNO + 19,
	POSIX_EMFILE = -EMFILE,
	POSIX_EMLINK = -EMLINK,
	POSIX_EMSGSIZE = __TOP_ERRNO + 20,
	POSIX_EMULTIHOP = __TOP_ERRNO + 21,
	POSIX_ENAMETOOLONG = -ENAMETOOLONG,
	POSIX_ENETDOWN = __TOP_ERRNO + 22,
	POSIX_ENETRESET = __TOP_ERRNO + 23,
	POSIX_ENETUNREACH = __TOP_ERRNO + 24,
	POSIX_ENFILE = __TOP_ERRNO + 25,
	POSIX_ENOBUFS = __TOP_ERRNO + 26,
	POSIX_ENODATA = -NO_DATA,
	POSIX_ENODEV = __TOP_ERRNO + 27,
	POSIX_ENOENT = -ENOENT,
	POSIX_ENOEXEC = __TOP_ERRNO + 28,
	POSIX_ENOLCK = __TOP_ERRNO + 29,
	POSIX_ENOLINK = __TOP_ERRNO + 30,
	POSIX_ENOMEM = -ENOMEM,
	POSIX_ENOMSG = __TOP_ERRNO + 31,
	POSIX_ENOPROTOOPT = __TOP_ERRNO + 32,
	POSIX_ENOSPC = -ENOSPC,
	POSIX_ENOSR = __TOP_ERRNO + 33,
	POSIX_ENOSTR = __TOP_ERRNO + 34,
	POSIX_ENOSYS = __TOP_ERRNO + 35,
	POSIX_ENOTCONN = -ENOTCONN,
	POSIX_ENOTDIR = -ENOTDIR,
	POSIX_ENOTEMPTY = -ENOTEMPTY,
	POSIX_ENOTRECOVERABLE = __TOP_ERRNO + 36,
	POSIX_ENOTSOCK = -ENOTSOCK,
	POSIX_ENOTSUP = -ENOTSUP,
	POSIX_ENOTTY = __TOP_ERRNO + 37,
	POSIX_ENXIO = __TOP_ERRNO + 38,
	POSIX_EOPNOTSUPP = __TOP_ERRNO + 39,
	POSIX_EOVERFLOW = -EOVERFLOW,
	POSIX_EOWNERDEAD = __TOP_ERRNO + 40,
	POSIX_EPERM = -EPERM,
	POSIX_EPIPE = __TOP_ERRNO + 41,
	POSIX_EPROTO = __TOP_ERRNO + 42,
	POSIX_EPROTONOSUPPORT = -EPROTONOSUPPORT,
	POSIX_EPROTOTYPE = __TOP_ERRNO + 43,
	POSIX_ERANGE = -ERANGE,
	POSIX_EROFS = __TOP_ERRNO + 44,
	POSIX_ESPIPE = __TOP_ERRNO + 45,
	POSIX_ESRCH = __TOP_ERRNO + 46,
	POSIX_ESTALE = __TOP_ERRNO + 47,
	POSIX_ETIME = __TOP_ERRNO + 48,
	POSIX_ETIMEDOUT = __TOP_ERRNO + 49,
	POSIX_ETXTBSY = __TOP_ERRNO + 50,
	POSIX_EWOULDBLOCK = __TOP_ERRNO + 51,
	POSIX_EXDEV = -EXDEV,
};

#undef __TOP_ERRNO

#undef E2BIG
#undef EACCES
#undef EADDRINUSE
#undef EADDRNOTAVAIL
#undef EAFNOSUPPORT
#undef EAGAIN
#undef EALREADY
#undef EBADF
#undef EBADMSG
#undef EBUSY
#undef ECANCELED
#undef ECHILD
#undef ECONNABORTED
#undef ECONNREFUSED
#undef ECONNRESET
#undef EDEADLK
#undef EDESTADDRREQ
#undef EDOM
#undef EDQUOT
#undef EEXIST
#undef EFAULT
#undef EFBIG
#undef EHOSTUNREACH
#undef EIDRM
#undef EILSEQ
#undef EINPROGRESS
#undef EINTR
#undef EINVAL
#undef EIO
#undef EISCONN
#undef EISDIR
#undef ELOOP
#undef EMFILE
#undef EMLINK
#undef EMSGSIZE
#undef EMULTIHOP
#undef ENAMETOOLONG
#undef ENETDOWN
#undef ENETRESET
#undef ENETUNREACH
#undef ENFILE
#undef ENOBUFS
#undef ENODATA
#undef ENODEV
#undef ENOENT
#undef ENOEXEC
#undef ENOLCK
#undef ENOLINK
#undef ENOMEM
#undef ENOMSG
#undef ENOPROTOOPT
#undef ENOSPC
#undef ENOSR
#undef ENOSTR
#undef ENOSYS
#undef ENOTCONN
#undef ENOTDIR
#undef ENOTEMPTY
#undef ENOTRECOVERABLE
#undef ENOTSOCK
#undef ENOTSUP
#undef ENOTTY
#undef ENXIO
#undef EOPNOTSUPP
#undef EOVERFLOW
#undef EOWNERDEAD
#undef EPERM
#undef EPIPE
#undef EPROTO
#undef EPROTONOSUPPORT
#undef EPROTOTYPE
#undef ERANGE
#undef EROFS
#undef ESPIPE
#undef ESRCH
#undef ESTALE
#undef ETIME
#undef ETIMEDOUT
#undef ETXTBSY
#undef EWOULDBLOCK
#undef EXDEV

#define E2BIG POSIX_E2BIG
#define EACCES POSIX_EACCES
#define EADDRINUSE POSIX_EADDRINUSE
#define EADDRNOTAVAIL POSIX_EADDRNOTAVAIL
#define EAFNOSUPPORT POSIX_EAFNOSUPPORT
#define EAGAIN POSIX_EAGAIN
#define EALREADY POSIX_EALREADY
#define EBADF POSIX_EBADF
#define EBADMSG POSIX_EBADMSG
#define EBUSY POSIX_EBUSY
#define ECANCELED POSIX_ECANCELED
#define ECHILD POSIX_ECHILD
#define ECONNABORTED POSIX_ECONNABORTED
#define ECONNREFUSED POSIX_ECONNREFUSED
#define ECONNRESET POSIX_ECONNRESET
#define EDEADLK POSIX_EDEADLK
#define EDESTADDRREQ POSIX_EDESTADDRREQ
#define EDOM POSIX_EDOM
#define EDQUOT POSIX_EDQUOT
#define EEXIST POSIX_EEXIST
#define EFAULT POSIX_EFAULT
#define EFBIG POSIX_EFBIG
#define EHOSTUNREACH POSIX_EHOSTUNREACH
#define EIDRM POSIX_EIDRM
#define EILSEQ POSIX_EILSEQ
#define EINPROGRESS POSIX_EINPROGRESS
#define EINTR POSIX_EINTR
#define EINVAL POSIX_EINVAL
#define EIO POSIX_EIO
#define EISCONN POSIX_EISCONN
#define EISDIR POSIX_EISDIR
#define ELOOP POSIX_ELOOP
#define EMFILE POSIX_EMFILE
#define EMLINK POSIX_EMLINK
#define EMSGSIZE POSIX_EMSGSIZE
#define EMULTIHOP POSIX_EMULTIHOP
#define ENAMETOOLONG POSIX_ENAMETOOLONG
#define ENETDOWN POSIX_ENETDOWN
#define ENETRESET POSIX_ENETRESET
#define ENETUNREACH POSIX_ENETUNREACH
#define ENFILE POSIX_ENFILE
#define ENOBUFS POSIX_ENOBUFS
#define ENODATA POSIX_ENODATA
#define ENODEV POSIX_ENODEV
#define ENOENT POSIX_ENOENT
#define ENOEXEC POSIX_ENOEXEC
#define ENOLCK POSIX_ENOLCK
#define ENOLINK POSIX_ENOLINK
#define ENOMEM POSIX_ENOMEM
#define ENOMSG POSIX_ENOMSG
#define ENOPROTOOPT POSIX_ENOPROTOOPT
#define ENOSPC POSIX_ENOSPC
#define ENOSR POSIX_ENOSR
#define ENOSTR POSIX_ENOSTR
#define ENOSYS POSIX_ENOSYS
#define ENOTCONN POSIX_ENOTCONN
#define ENOTDIR POSIX_ENOTDIR
#define ENOTEMPTY POSIX_ENOTEMPTY
#define ENOTRECOVERABLE POSIX_ENOTRECOVERABLE
#define ENOTSOCK POSIX_ENOTSOCK
#define ENOTSUP POSIX_ENOTSUP
#define ENOTTY POSIX_ENOTTY
#define ENXIO POSIX_ENXIO
#define EOPNOTSUPP POSIX_EOPNOTSUPP
#define EOVERFLOW POSIX_EOVERFLOW
#define EOWNERDEAD POSIX_EOWNERDEAD
#define EPERM POSIX_EPERM
#define EPIPE POSIX_EPIPE
#define EPROTO POSIX_EPROTO
#define EPROTONOSUPPORT POSIX_EPROTONOSUPPORT
#define EPROTOTYPE POSIX_EPROTOTYPE
#define ERANGE POSIX_ERANGE
#define EROFS POSIX_EROFS
#define ESPIPE POSIX_ESPIPE
#define ESRCH POSIX_ESRCH
#define ESTALE POSIX_ESTALE
#define ETIME POSIX_ETIME
#define ETIMEDOUT POSIX_ETIMEDOUT
#define ETXTBSY POSIX_ETXTBSY
#define EWOULDBLOCK POSIX_EWOULDBLOCK
#define EXDEV POSIX_EXDEV

#endif /* POSIX_ERRNO_H_ */

/** @}
 */
