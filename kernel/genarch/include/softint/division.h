/*
 * Copyright (c) 2006 Josef Cejka
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

/** @addtogroup genarch
 * @{
 */
/** @file
 */

#ifndef KERN_DIVISION_H_
#define KERN_DIVISION_H_

/* 32bit integer division */
int __divsi3(int a, int b);

/* 64bit integer division */
long long __divdi3(long long a, long long b);

/* 32bit unsigned integer division */
unsigned int __udivsi3(unsigned int a, unsigned int b);

/* 64bit unsigned integer division */
unsigned long long __udivdi3(unsigned long long a, unsigned long long b);

/* 32bit remainder of the signed division */
int __modsi3(int a, int b);

/* 64bit remainder of the signed division */
long long __moddi3(long long a, long long b);

/* 32bit remainder of the unsigned division */
unsigned int __umodsi3(unsigned int a, unsigned int b);

/* 64bit remainder of the unsigned division */
unsigned long long __umoddi3(unsigned long long a, unsigned long long b);

unsigned long long __udivmoddi3(unsigned long long a, unsigned long long b, unsigned long long *c); 

#endif

/** @}
 */
