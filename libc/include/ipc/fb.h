

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

#define FB_DRAW_PPM          1038
#define FB_PREPARE_SHM       1039
#define FB_DROP_SHM          1040
#define FB_SHM2PIXMAP        1041

#define FB_VP_DRAW_PIXMAP    1042
#define FB_VP2PIXMAP         1043
#define FB_DROP_PIXMAP       1044

#define FB_TRANS_PUTCHAR     1045

#define FB_ANIM_CREATE       1046
#define FB_ANIM_DROP         1047
#define FB_ANIM_ADDPIXMAP    1048
#define FB_ANIM_CHGVP        1049
#define FB_ANIM_START        1050
#define FB_ANIM_STOP         1051

#endif
