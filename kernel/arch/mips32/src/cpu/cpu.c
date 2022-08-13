/*
 * SPDX-FileCopyrightText: 2003-2004 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_mips32
 * @{
 */
/** @file
 */

#include <arch/cpu.h>
#include <cpu.h>
#include <arch.h>
#include <arch/cp0.h>
#include <stdio.h>

struct data_t {
	const char *vendor;
	const char *model;
};

static struct data_t imp_data[] = {
	{ "Invalid", "Invalid" },	/* 0x00 */
	{ "MIPS", "R2000" },		/* 0x01 */
	{ "MIPS", "R3000" },		/* 0x02 */
	{ "MIPS", "R6000" },		/* 0x03 */
	{ "MIPS", "R4000/R4400" }, 	/* 0x04 */
	{ "LSI Logic", "R3000" },	/* 0x05 */
	{ "MIPS", "R6000A" },		/* 0x06 */
	{ "IDT", "3051/3052" },		/* 0x07 */
	{ "Invalid", "Invalid" },	/* 0x08 */
	{ "MIPS", "R10000/T5" },	/* 0x09 */
	{ "MIPS", "R4200" },		/* 0x0a */
	{ "Unknown", "Unknown" },	/* 0x0b */
	{ "Unknown", "Unknown" },	/* 0x0c */
	{ "Invalid", "Invalid" },	/* 0x0d */
	{ "Invalid", "Invalid" },	/* 0x0e */
	{ "Invalid", "Invalid" },	/* 0x0f */
	{ "MIPS", "R8000" },		/* 0x10 */
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
	{ "Sony", "R3000" },		/* 0x21 */
	{ "Toshiba", "R3000" },		/* 0x22 */
	{ "NKK", "R3000" },		/* 0x23 */
	{ NULL, NULL }
};

static struct data_t imp_data80[] = {
	{ "MIPS", "4Kc" },  /* 0x80 */
	{ "Invalid", "Invalid" }, /* 0x81 */
	{ "Invalid", "Invalid" }, /* 0x82 */
	{ "MIPS", "4Km & 4Kp" }, /* 0x83 */
	{ NULL, NULL }
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
	struct data_t *data;
	unsigned int i;

	if (m->arch.imp_num & 0x80) {
		/* Count records */
		i = 0;
		while (imp_data80[i].vendor)
			i++;
		if ((m->arch.imp_num & 0x7f) >= i) {
			printf("imp=%d\n", m->arch.imp_num);
			return;
		}
		data = &imp_data80[m->arch.imp_num & 0x7f];
	} else {
		i = 0;
		while (imp_data[i].vendor)
			i++;
		if (m->arch.imp_num >= i) {
			printf("imp=%d\n", m->arch.imp_num);
			return;
		}
		data = &imp_data[m->arch.imp_num];
	}

	printf("cpu%u: %s %s (rev=%d.%d, imp=%d)\n",
	    m->id, data->vendor, data->model, m->arch.rev_num >> 4,
	    m->arch.rev_num & 0x0f, m->arch.imp_num);
}

/** @}
 */
