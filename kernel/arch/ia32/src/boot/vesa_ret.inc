/*
 * SPDX-FileCopyrightText: 2022 HelenOS Project
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

.code32
vesa_init_protected:
	cli
	cld

	/* Initialize stack pointer */
	movl $START_STACK, %esp

	/* Kernel data + stack */
	movw $GDT_SELECTOR(KDATA_DES), %cx
	movw %cx, %es
	movw %cx, %fs
	movw %cx, %gs
	movw %cx, %ds
	movw %cx, %ss

	jmpl $GDT_SELECTOR(KTEXT_DES), $vesa_meeting_point
