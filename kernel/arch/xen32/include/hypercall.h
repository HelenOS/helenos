/*
 * Copyright (C) 2006 Martin Decky
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

#ifndef __xen32_HYPERCALL_H__
#define __xen32_HYPERCALL_H__

#include <arch/types.h>
#include <macros.h>


#define XEN_CONSOLE_IO	18


/*
 * Commands for XEN_CONSOLE_IO
 */
#define CONSOLE_IO_WRITE	0
#define CONSOLE_IO_READ		1


#define hypercall0(id)	\
	({	\
		unative_t ret;	\
		asm volatile (	\
			"call hypercall_page + (" STRING(id) " * 32)\n"	\
			: "=a" (ret)	\
			:	\
			: "memory"	\
		);	\
		ret;	\
	})

#define hypercall1(id, p1)	\
	({	\
		unative_t ret, __ign1;	\
		asm volatile (	\
			"call hypercall_page + (" STRING(id) " * 32)\n"	\
			: "=a" (ret), \
			  "=b" (__ign1)	\
			: "1" (p1)	\
			: "memory"	\
		);	\
		ret;	\
	})

#define hypercall2(id, p1, p2)	\
	({	\
		unative_t ret, __ign1, __ign2;	\
		asm volatile (	\
			"call hypercall_page + (" STRING(id) " * 32)\n"	\
			: "=a" (ret), \
			  "=b" (__ign1),	\
			  "=c" (__ign2)	\
			: "1" (p1),	\
			  "2" (p2)	\
			: "memory"	\
		);	\
		ret;	\
	})

#define hypercall3(id, p1, p2, p3)	\
	({	\
		unative_t ret, __ign1, __ign2, __ign3;	\
		asm volatile (	\
			"call hypercall_page + (" STRING(id) " * 32)\n"	\
			: "=a" (ret), \
			  "=b" (__ign1),	\
			  "=c" (__ign2),	\
			  "=d" (__ign3)	\
			: "1" (p1),	\
			  "2" (p2),	\
			  "3" (p3)	\
			: "memory"	\
		);	\
		ret;	\
	})

#define hypercall4(id, p1, p2, p3, p4)	\
	({	\
		unative_t ret, __ign1, __ign2, __ign3, __ign4;	\
		asm volatile (	\
			"call hypercall_page + (" STRING(id) " * 32)\n"	\
			: "=a" (ret), \
			  "=b" (__ign1),	\
			  "=c" (__ign2),	\
			  "=d" (__ign3),	\
			  "=S" (__ign4)	\
			: "1" (p1),	\
			  "2" (p2),	\
			  "3" (p3),	\
			  "4" (p4)	\
			: "memory"	\
		);	\
		ret;	\
	})

#define hypercall5(id, p1, p2, p3, p4, p5)	\
	({	\
		unative_t ret, __ign1, __ign2, __ign3, __ign4, __ign5;	\
		asm volatile (	\
			"call hypercall_page + (" STRING(id) " * 32)\n"	\
			: "=a" (ret), \
			  "=b" (__ign1),	\
			  "=c" (__ign2),	\
			  "=d" (__ign3),	\
			  "=S" (__ign4),	\
			  "=D" (__ign5)	\
			: "1" (p1),	\
			  "2" (p2),	\
			  "3" (p3),	\
			  "4" (p4),	\
			  "5" (p5)	\
			: "memory"	\
		);	\
		ret;	\
	})


static inline int xen_console_io(const int cmd, const int count, const char *str)
{
	return hypercall3(XEN_CONSOLE_IO, cmd, count, str);
}

#endif
