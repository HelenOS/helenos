/*
 * Copyright (C) 2005 Josef Cejka
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
#include <print.h>
#include <test.h>

void test(void)
{
	__u64 u64const = 0x0123456789ABCDEFLL;
	printf(" Printf test \n");
	printf(" Q  %Q  %q \n",u64const, u64const);
	printf(" L  %L  %l \n",0x01234567 ,0x01234567);   
	printf(" W  %W  %w \n",0x0123 ,0x0123);   
	printf(" B  %B  %b \n",0x01 ,0x01);
	printf(" F  %F	%f (123456789.987654321)\n",123456789.987654321,123456789.987654321);
	printf(" F  %F	%f (-123456789.987654321e-10)\n",-123456789.987654321e-10,-123456789.987654321e-10);
	printf(" E  %E	%e (123456789.987654321)\n",123456789.987654321,123456789.987654321);
	printf(" E  %.10E %.8e (-123456789.987654321e-12 for precision 10 & 8)\n",-123456789.987654321e-12,-123456789.987654321e-12);
	printf(" E  %.10E %.8e (-987654321.123456789e-12 for precision 10 & 8)\n",-987654321.123456789e-12,-987654321.123456789e-12);
	printf(" E  %.10E %.8e (123456789.987654321e-12 for precision 10 & 8)\n",123456789.987654321e-12,123456789.987654321e-12);
	printf(" E  %.10E %.8e (987654321.123456789e-12 for precision 10 & 8)\n",987654321.123456789e-12,987654321.123456789e-12);
	printf(" E  %.10E %.8e (-123456789.987654321e12 for precision 10 & 8)\n",-123456789.987654321e12,-123456789.987654321e12);
	printf(" E  %.10E %.8e (-987654321.123456789e12 for precision 10 & 8)\n",-987654321.123456789e12,-987654321.123456789e12);
	printf(" E  %.10E %.8e (123456789.987654321e12 for precision 10 & 8)\n",123456789.987654321e12,123456789.987654321e12);
	printf(" E  %.10E %.8e (987654321.123456789e12 for precision 10 & 8)\n",987654321.123456789e12,987654321.123456789e12);
	return;
}
