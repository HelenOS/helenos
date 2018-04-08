#ifndef LIBCPP_INTERNAL_COMMON
#define LIBCPP_INTERNAL_COMMON

/**
 * According to section 17.2 of the standard,
 * the restrict qualifier shall be omitted.
 */
#define restrict

#undef NULL
#define NULL nullptr

#endif
