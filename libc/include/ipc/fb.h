

#include <arch/types.h>
#include <types.h>

#ifndef __libc__FB_H__
#define __libc__FB_H__

#define FB_PUTCHAR           1025
#define FB_CLEAR             1026
#define FB_GET_CSIZE         1027
#define FB_CURSOR_VISIBILITY 1028
#define FB_CURSOR_GOTO       1029
#define FB_SCROLL            1030
#define FB_VIEWPORT_SWITCH   1031
#define FB_VIEWPORT_CREATE   1032
#define FB_VIEWPORT_DELETE   1033
#define FB_SET_STYLE         1034
#define FB_GET_RESOLUTION    1035
#define FB_DRAW_TEXT_DATA    1036
#define FB_FLUSH             1037

#endif
