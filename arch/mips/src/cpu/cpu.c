/*
 * Copyright (C) 2001-2004 Jakub Jermar
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

#include <arch/cpu.h>
#include <cpu.h>

#include <arch.h>

#include <arch/cp0.h>

#include <typedefs.h>

struct {
	char *vendor;
	char *model;
} imp_data[] = {
    { "Invalid", "Invalid" },	/* 0x00 */
    { "MIPS", "R2000" },	/* 0x01 */
    { "MIPS", "R3000" },	/* 0x02 */
    { "MIPS", "R6000" },	/* 0x03 */
    { "MIPS", " R4000/R4400" }, /* 0x04 */
    { "LSI Logic", "R3000" },	/* 0x05 */
    { "MIPS", "R6000A" },	/* 0x06 */
    { "IDT", "3051/3052" },	/* 0x07 */
    { "Invalid", "Invalid" },	/* 0x08 */
    { "MIPS", "R10000/T5" },	/* 0x09 */
    { "MIPS", "R4200" },	/* 0x0a */
    { "Unknown", "Unknown" },	/* 0x0b */
    { "Unknown", "Unknown" },	/* 0x0c */
    { "Invalid", "Invalid" },	/* 0x0d */
    { "Invalid", "Invalid" },	/* 0x0e */
    { "Invalid", "Invalid" },	/* 0x0f */
    { "MIPS", "R8000" },	/* 0x10 */
    { "Invalid", "Invalid" },	/* 0x11 */
    { "Invalid", "Invalid" },	/* 0x12 */
    { "Invalid", "Invalid" },	/* 0x13 */
    { "Invalid", "Invalid" },	/* 0x14 */
    { "Invalid", "Invalid" },	/* 0x15 */
    { "Invalid", "Invalid" },	/* 0x16 */
    { "Invalid", "Invalid" },	/* 0x17 */
    { "Invalid", "Invalid" },	/* 0x18 */
    { "Invalid", "Invalid" },	/* 0x19 */
    { "Invalid", "Invalid" },  	/* 0x1a */
    { "Invalid", "Invalid" },	/* 0x1b */
    { "Invalid", "Invalid" },	/* 0x1c */
    { "Invalid", "Invalid" },	/* 0x1d */
    { "Invalid", "Invalid" },	/* 0x1e */
    { "Invalid", "Invalid" },	/* 0x1f */
    { "QED", "R4600" },		/* 0x20 */
    { "Sony", "R3000" },	/* 0x21 */
    { "Toshiba", "R3000" },	/* 0x22 */
    { "NKK", "R3000" }		/* 0x23 */
};

void cpu_arch_init(void)
{
}

void cpu_identify(void)
{
	CPU->arch.rev_num = cp0_prid_read() & 0xff;
	CPU->arch.imp_num = (cp0_prid_read() >> 8) & 0xff;
}

void cpu_print_report(cpu_t *m)
{
	printf("cpu%d: %s %s (rev=%d.%d, imp=%d)\n",
		m->id, imp_data[m->arch.imp_num].vendor, imp_data[m->arch.imp_num].model, m->arch.rev_num >> 4, m->arch.rev_num & 0xf, m->arch.imp_num);
}
