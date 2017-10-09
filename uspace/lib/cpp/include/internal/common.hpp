#ifndef LIBCPP_INTERNAL_COMMON
#define LIBCPP_INTERNAL_COMMON

/**
 * G++ is will not properly include libc
 * headers without this define.
 */
#define _Bool bool

/**
 * The restrict keyword is not part of the
 * C++ standard, but g++ supports __restrict__,
 * this might cause problems with other compilers
 * like clang.
 * TODO: Test this.
 */
#define restrict __restrict__

#endif
