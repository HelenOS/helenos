#ifndef LIBCPP_INTERNAL_COMMON
#define LIBCPP_INTERNAL_COMMON

/**
 * The restrict keyword is not part of the
 * C++ standard, but g++ supports __restrict__,
 * this might cause problems with other compilers
 * like clang.
 * TODO: Test this.
 */
#define restrict __restrict__

#undef NULL
#define NULL nullptr

#endif
