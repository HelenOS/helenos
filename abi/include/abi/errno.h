/* Generated file. Edit errno.in instead. */
/*
 * SPDX-FileCopyrightText: 2022 HelenOS Project
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#define EOK             __errno_t(      0)
#define ENOENT          __errno_t(      1)
#define ENOMEM          __errno_t(      2)
#define ELIMIT          __errno_t(      3)
#define EREFUSED        __errno_t(      4)
#define EFORWARD        __errno_t(      5)
#define EPERM           __errno_t(      6)
#define EHANGUP         __errno_t(      7)
#define EPARTY          __errno_t(      8)
#define EEXIST          __errno_t(      9)
#define EBADMEM         __errno_t(     10)
#define ENOTSUP         __errno_t(     11)
#define EADDRNOTAVAIL   __errno_t(     12)
#define ETIMEOUT        __errno_t(     13)
#define EINVAL          __errno_t(     14)
#define EBUSY           __errno_t(     15)
#define EOVERFLOW       __errno_t(     16)
#define EINTR           __errno_t(     17)
#define EMFILE          __errno_t(     18)
#define ENAMETOOLONG    __errno_t(     19)
#define EISDIR          __errno_t(     20)
#define ENOTDIR         __errno_t(     21)
#define ENOSPC          __errno_t(     22)
#define ENOTEMPTY       __errno_t(     23)
#define EBADF           __errno_t(     24)
#define EDOM            __errno_t(     25)
#define ERANGE          __errno_t(     26)
#define EXDEV           __errno_t(     27)
#define EIO             __errno_t(     28)
#define EMLINK          __errno_t(     29)
#define ENXIO           __errno_t(     30)
#define ENOFS           __errno_t(     31)
#define EBADCHECKSUM    __errno_t(     32)
#define ESTALL          __errno_t(     33)
#define EEMPTY          __errno_t(     34)
#define ENAK            __errno_t(     35)
#define EAGAIN          __errno_t(     36)

/* libhttp error codes. Defining them here is a temporary hack. */

#define HTTP_EMULTIPLE_HEADERS  __errno_t(  50)
#define HTTP_EMISSING_HEADER    __errno_t(  51)
#define HTTP_EPARSE             __errno_t(  52)

/* libext4 error codes. Same as above. */

#define EXT4_ERR_BAD_DX_DIR     __errno_t(  60)

/*
 * POSIX error codes + whatever nonstandard names crop up in third-party
 * software. These are not used in HelenOS code, but are defined here in
 * order to avoid nasty hacks in libposix.
 *
 * If you decide to use one of these in native HelenOS code,
 * move it up to the first group.
 */

/* COMPAT_START -- do not remove or edit this comment */

#define E2BIG             __errno_t(  101)
#define EACCES            __errno_t(  102)
#define EADDRINUSE        __errno_t(  103)
#define EAFNOSUPPORT      __errno_t(  105)
#define EALREADY          __errno_t(  107)
#define EBADMSG           __errno_t(  109)
#define ECANCELED         __errno_t(  111)
#define ECHILD            __errno_t(  112)
#define ECONNABORTED      __errno_t(  113)
#define ECONNREFUSED      __errno_t(  114)
#define ECONNRESET        __errno_t(  115)
#define EDEADLK           __errno_t(  116)
#define EDESTADDRREQ      __errno_t(  117)
#define EDQUOT            __errno_t(  119)
#define EFAULT            __errno_t(  121)
#define EFBIG             __errno_t(  122)
#define EHOSTUNREACH      __errno_t(  123)
#define EIDRM             __errno_t(  124)
#define EILSEQ            __errno_t(  125)
#define EINPROGRESS       __errno_t(  126)
#define EISCONN           __errno_t(  130)
#define ELOOP             __errno_t(  132)
#define EMSGSIZE          __errno_t(  135)
#define EMULTIHOP         __errno_t(  136)
#define ENETDOWN          __errno_t(  138)
#define ENETRESET         __errno_t(  139)
#define ENETUNREACH       __errno_t(  140)
#define ENFILE            __errno_t(  141)
#define ENOBUFS           __errno_t(  142)
#define ENODATA           __errno_t(  143)
#define ENODEV            __errno_t(  144)
#define ENOEXEC           __errno_t(  146)
#define ENOLCK            __errno_t(  147)
#define ENOLINK           __errno_t(  148)
#define ENOMSG            __errno_t(  150)
#define ENOPROTOOPT       __errno_t(  151)
#define ENOSR             __errno_t(  153)
#define ENOSTR            __errno_t(  154)
#define ENOSYS            __errno_t(  155)
#define ENOTCONN          __errno_t(  156)
#define ENOTRECOVERABLE   __errno_t(  159)
#define ENOTSOCK          __errno_t(  160)
#define ENOTTY            __errno_t(  162)
#define EOPNOTSUPP        __errno_t(  164)
#define EOWNERDEAD        __errno_t(  166)
#define EPIPE             __errno_t(  168)
#define EPROTO            __errno_t(  169)
#define EPROTONOSUPPORT   __errno_t(  170)
#define EPROTOTYPE        __errno_t(  171)
#define EROFS             __errno_t(  173)
#define ESPIPE            __errno_t(  174)
#define ESRCH             __errno_t(  175)
#define ESTALE            __errno_t(  176)
#define ETIME             __errno_t(  177)
#define ETIMEDOUT         __errno_t(  178)
#define ETXTBSY           __errno_t(  179)
#define EWOULDBLOCK       __errno_t(  180)
