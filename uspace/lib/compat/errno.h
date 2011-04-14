
#ifndef COMPAT_ERRNO_H
#define COMPAT_ERRNO_H

#include "../../c/include/errno.h"

// TODO: HelenOS has negative error codes, while C standard dictates
//       positive. Does any sane application depend on this, or can
//       we just leave it as is?

#endif // COMPAT_ERRNO_H

