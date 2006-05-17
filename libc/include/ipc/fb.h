

#include <arch/types.h>
#include <types.h>

#ifndef __libc__FB_H__
#define __libc__FB_H__

#define FB_GET_VFB 1024
#define FB_PUTCHAR 1025

#define METHOD_WIDTH 16
#define ITEM_WIDTH 16
#define COUNT_WIDTH 16 /* Should be 8 times integer */


struct _fb_method {
	unsigned m : METHOD_WIDTH;
	unsigned item : ITEM_WIDTH;
} __attribute__((packed));

union fb_method {
	struct _fb_method m;
	__native fill;
} __attribute__((packed));

struct fb_call_args {
	union fb_method method;
	union {
		struct {
			unsigned count : COUNT_WIDTH;
			char chars[3 * sizeof(__native) - (COUNT_WIDTH >> 3)];
		} putchar __attribute__((packed));
	} data ; // __attribute__((packed));	
} __attribute__((packed));

struct fb_ipc_args {
	__native method;
	__native arg1;
	__native arg2;
	__native arg3;
} __attribute__((packed));

union fb_args {
	struct fb_call_args fb_args;
	struct fb_ipc_args ipc_args;
} __attribute__((packed));

typedef union fb_args fb_args_t;

#endif
