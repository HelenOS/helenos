/*
 * SPDX-FileCopyrightText: 2005 Jakub Jermar
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <abi/asmtool.h>
#include <arch/arch.h>
#include <arch/stack.h>

.text

.register %g2, #scratch
.register %g3, #scratch

/*
 * Almost the same as memcpy() except the loads are from userspace.
 */
FUNCTION_BEGIN(memcpy_from_uspace)
	mov %o0, %o3  /* save dst */
	add %o1, 7, %g1
	and %g1, -8, %g1
	cmp %o1, %g1
	be,pn %xcc, 3f
	add %o0, 7, %g1
	mov 0, %g3

	0:

		brz,pn %o2, 2f
		mov 0, %g2

	1:

		lduba [%g3 + %o1] ASI_AIUS, %g1
		add %g2, 1, %g2
		cmp %o2, %g2
		stb %g1, [%g3 + %o0]
		bne,pt %xcc, 1b
		mov %g2, %g3

	2:

		jmp %o7 + 8  /* exit point */
		mov %o3, %o0

	3:

		and %g1, -8, %g1
		cmp %o0, %g1
		bne,pt %xcc, 0b
		mov 0, %g3
		srlx %o2, 3, %g4
		brz,pn %g4, 5f
		mov 0, %g5

	4:

		sllx %g3, 3, %g2
		add %g5, 1, %g3
		ldxa [%o1 + %g2] ASI_AIUS, %g1
		mov %g3, %g5
		cmp %g4, %g3
		bne,pt %xcc, 4b
		stx %g1, [%o0 + %g2]

	5:

		and %o2, 7, %o2
		brz,pn %o2, 2b
		sllx %g4, 3, %g1
		mov 0, %g2
		add %g1, %o0, %o0
		add %g1, %o1, %g4
		mov 0, %g3

	6:

		lduba [%g2 + %g4] ASI_AIUS, %g1
		stb %g1, [%g2 + %o0]
		add %g3, 1, %g2
		cmp %o2, %g2
		bne,pt %xcc, 6b
		mov %g2, %g3

		jmp %o7 + 8  /* exit point */
		mov %o3, %o0
FUNCTION_END(memcpy_from_uspace)

/*
 * Almost the same as memcpy() except the stores are to userspace.
 */
FUNCTION_BEGIN(memcpy_to_uspace)
	mov %o0, %o3  /* save dst */
	add %o1, 7, %g1
	and %g1, -8, %g1
	cmp %o1, %g1
	be,pn %xcc, 3f
	add %o0, 7, %g1
	mov 0, %g3

	0:

		brz,pn %o2, 2f
		mov 0, %g2

	1:

		ldub [%g3 + %o1], %g1
		add %g2, 1, %g2
		cmp %o2, %g2
		stba %g1, [%g3 + %o0] ASI_AIUS
		bne,pt %xcc, 1b
		mov %g2, %g3

	2:

		jmp %o7 + 8  /* exit point */
		mov %o3, %o0

	3:

		and %g1, -8, %g1
		cmp %o0, %g1
		bne,pt %xcc, 0b
		mov 0, %g3
		srlx %o2, 3, %g4
		brz,pn %g4, 5f
		mov 0, %g5

	4:

		sllx %g3, 3, %g2
		add %g5, 1, %g3
		ldx [%o1 + %g2], %g1
		mov %g3, %g5
		cmp %g4, %g3
		bne,pt %xcc, 4b
		stxa %g1, [%o0 + %g2] ASI_AIUS

	5:

		and %o2, 7, %o2
		brz,pn %o2, 2b
		sllx %g4, 3, %g1
		mov 0, %g2
		add %g1, %o0, %o0
		add %g1, %o1, %g4
		mov 0, %g3

	6:

		ldub [%g2 + %g4], %g1
		stba %g1, [%g2 + %o0] ASI_AIUS
		add %g3, 1, %g2
		cmp %o2, %g2
		bne,pt %xcc, 6b
		mov %g2, %g3

		jmp	%o7 + 8  /* exit point */
		mov	%o3, %o0
FUNCTION_END(memcpy_to_uspace)

SYMBOL(memcpy_from_uspace_failover_address)
SYMBOL(memcpy_to_uspace_failover_address)
	jmp %o7 + 8   /* exit point */
	mov %g0, %o0  /* return 0 on failure */

FUNCTION_BEGIN(early_putuchar)
	retl
	nop
FUNCTION_END(early_putuchar)
