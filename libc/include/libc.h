/*
 * Copyright (C) 2005 Martin Decky
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

#ifndef __LIBC__LIBC_H__
#define __LIBC__LIBC_H__

#include <types.h>

#include <kernel/syscall/syscall.h>
#include <kernel/arch/mm/page.h>

#define __SYSCALL0(id) __syscall(0, 0, 0, 0, id)
#define __SYSCALL1(id, p1) __syscall(p1, 0, 0, 0, id)
#define __SYSCALL2(id, p1, p2) __syscall(p1, p2, 0, 0, id)
#define __SYSCALL3(id, p1, p2, p3) __syscall(p1,p2,p3, 0, id)
#define __SYSCALL4(id, p1, p2, p3, p4) __syscall(p1,p2,p3,p4,id)

#define getpagesize()     (PAGE_SIZE)

extern void __main(void);
extern void __exit(void);
extern sysarg_t __syscall(const sysarg_t p1, const sysarg_t p2, 
			  const sysarg_t p3, const sysarg_t p4,
			  const syscall_t id);


#endif
