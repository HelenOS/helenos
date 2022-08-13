/*
 * SPDX-FileCopyrightText: 2007 Michal Kebrt, Pavel Jancik
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>

.text

.org 0

## User-space task entry point
#
# r1 contains the PCB pointer
# r2 contains the RAS page address
#
SYMBOL(_start)
	# Store the RAS page address into the ras_page variable
	ldr r0, =ras_page
	str r2, [r0]

	#
	# Create the first stack frame.
	#
	mov fp, #0
	mov ip, sp
	push {fp, ip, lr, pc}
	sub fp, ip, #4

	# Pass pcb_ptr to __c_start as the first argument (in r0)
	mov r0, r1
	bl __c_start

.data
