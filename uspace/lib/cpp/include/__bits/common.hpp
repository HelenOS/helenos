#ifndef LIBCPP_BITS_COMMON
#define LIBCPP_BITS_COMMON

/**
 * According to section 17.2 of the standard,
 * the restrict qualifier shall be omitted.
 */
#define restrict

#undef NULL
#define NULL nullptr

#endif
