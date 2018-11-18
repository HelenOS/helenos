#ifdef CONFIG_FB

#include <macros.h>

#define VESA_INFO_SIZE 1024

#define VESA_MODE_ATTRIBUTES_OFFSET  0
#define VESA_MODE_LIST_PTR_OFFSET    14
#define VESA_MODE_SCANLINE_OFFSET    16
#define VESA_MODE_WIDTH_OFFSET       18
#define VESA_MODE_HEIGHT_OFFSET      20
#define VESA_MODE_BPP_OFFSET         25
#define VESA_MODE_RED_MASK_OFFSET    31
#define VESA_MODE_RED_POS_OFFSET     32
#define VESA_MODE_GREEN_MASK_OFFSET  33
#define VESA_MODE_GREEN_POS_OFFSET   34
#define VESA_MODE_BLUE_MASK_OFFSET   35
#define VESA_MODE_BLUE_POS_OFFSET    36
#define VESA_MODE_PHADDR_OFFSET      40

#define VESA_END_OF_MODES  0xffff

#define VESA_OK  0x4f

#define VESA_GET_INFO       0x4f00
#define VESA_GET_MODE_INFO  0x4f01
#define VESA_SET_MODE       0x4f02
#define VESA_SET_PALETTE    0x4f09

.code32
vesa_init:
	lidtl vesa_idtr

	mov $GDT_SELECTOR(VESA_INIT_DATA_DES), %bx

	mov %bx, %es
	mov %bx, %fs
	mov %bx, %gs
	mov %bx, %ds
	mov %bx, %ss

	jmp $GDT_SELECTOR(VESA_INIT_CODE_DES), $vesa_init_real - vesa_init

vesa_idtr:
	.word 0x3ff
	.long 0

.code16
vesa_init_real:
	mov %cr0, %eax
	and $~CR0_PE, %eax
	mov %eax, %cr0

	jmp $VESA_INIT_SEGMENT, $vesa_init_real2 - vesa_init

vesa_init_real2:
	mov $VESA_INIT_SEGMENT, %bx

	mov %bx, %es
	mov %bx, %fs
	mov %bx, %gs
	mov %bx, %ds
	mov %bx, %ss

	movl %esp, %eax
	movl $0x0000fffc, %esp
	movl $0x0000fffc, %ebp
	pushl %eax

	/* Parse default mode string */

	mov $default_mode - vesa_init, %di
	xor %eax, %eax
	xor %ebx, %ebx

	mov $8, %ecx
	parse_width:
		mov (%di), %al

		/* Check for digit */

		cmp $'0', %al
		jb parse_width_done

		cmp $'9', %al
		ja parse_width_done

		sub $'0', %al

		/* Multiply default_width by 10 and add digit */

		mov default_width - vesa_init, %bx
		lea (%ebx, %ebx, 4), %ebx
		shl $1, %ebx
		add %ax, %bx
		mov %bx, default_width - vesa_init

		inc %di
		loop parse_width
	parse_width_done:

	mov (%di), %al
	cmp $0, %al
	jz parse_done
	inc %di

	mov $8, %ecx
	parse_height:
		mov (%di), %al

		/* Check for digit */

		cmp $'0', %al
		jb parse_height_done

		cmp $'9', %al
		ja parse_height_done

		sub $'0', %al

		/* Multiply default_height by 10 and add digit */

		mov default_height - vesa_init, %bx
		lea (%ebx, %ebx, 4), %ebx
		shl $1, %ebx
		add %ax, %bx
		mov %bx, default_height - vesa_init

		inc %di
		loop parse_height
	parse_height_done:

	mov (%di), %al
	cmp $0, %al
	jz parse_done
	inc %di

	mov $4, %ecx
	parse_bpp:
		mov (%di), %al

		/* Check for digit */

		cmp $'0', %al
		jb parse_bpp_done

		cmp $'9', %al
		ja parse_bpp_done

		sub $'0', %al

		/* Multiply default_bpp by 10 and add digit */

		mov default_bpp - vesa_init, %bx
		lea (%ebx, %ebx, 4), %ebx
		shl $1, %ebx
		add %ax, %bx
		mov %bx, default_bpp - vesa_init

		inc %di
		loop parse_bpp
	parse_bpp_done:

	parse_done:

	mov $VESA_GET_INFO, %ax
	mov $e_vesa_init - vesa_init, %di
	push %di
	/* Write the "VBE2" signature into the info structure in order
	 * to get proper mode information. The presence of "VBE2"
	 * indicates two things:
	 *
	 *  - VBE controller information structure is expected to be
	 *    512 bytes long instead of 256 bytes.
	 *  - The BIOS should report VBE 3.0 information (potentially
	 *    including non-standard modes in the mode list).
	 */
	movl $0x32454256, (%di)
	int $0x10

	pop %di
	cmp $VESA_OK, %al
	jnz no_mode

	mov 2 + VESA_MODE_LIST_PTR_OFFSET(%di), %si
	mov %si, %gs
	mov VESA_MODE_LIST_PTR_OFFSET(%di), %si

	add $VESA_INFO_SIZE, %di

	next_mode:
		/* Try next mode */

		mov %gs:(%si), %cx
		cmp $VESA_END_OF_MODES, %cx
		je no_mode

		inc %si
		inc %si
		push %cx
		push %di
		push %si
		mov $VESA_GET_MODE_INFO, %ax
		int $0x10

		pop %si
		pop %di
		pop %cx
		cmp $VESA_OK, %al
		jne no_mode

		/*
		 * Check for proper attributes (supported,
		 * color, graphics, linear framebuffer).
		 */

		mov VESA_MODE_ATTRIBUTES_OFFSET(%di), %ax
		and $0x99, %ax
		cmp $0x99, %ax
		jne next_mode

		/* Check for proper resolution */

		mov default_width - vesa_init, %ax
		cmp VESA_MODE_WIDTH_OFFSET(%di), %ax
		jne next_mode

		mov default_height - vesa_init, %ax
		cmp VESA_MODE_HEIGHT_OFFSET(%di), %ax
		jne next_mode

		/* Check for proper bpp */

		mov default_bpp - vesa_init, %al
		cmp VESA_MODE_BPP_OFFSET(%di), %al
		je set_mode

		mov $24, %al
		cmp default_bpp - vesa_init, %al
		jne next_mode

		/* For 24 bpp modes accept also 32 bit bpp */

		mov $32, %al
		cmp VESA_MODE_BPP_OFFSET(%di), %al
		jne next_mode

	set_mode:
		mov %cx, %bx
		or $0xc000, %bx
		push %di
		mov $VESA_SET_MODE, %ax
		int $0x10

		pop %di
		cmp $VESA_OK, %al
		jnz no_mode

		/* Set 3:2:3 VGA palette */

		mov VESA_MODE_BPP_OFFSET(%di), %al
		cmp $8, %al
		jnz vga_not_set

		mov VESA_MODE_ATTRIBUTES_OFFSET(%di), %ax
		push %di
		mov $vga323 - vesa_init, %di
		mov $0x100, %ecx

		/* Test if VGA compatible registers are present */
		bt $5, %ax
		jnc vga_compat

			/* Use VESA routine to set the palette */

			mov $VESA_SET_PALETTE, %ax
			xor %bl, %bl
			xor %dx, %dx
			int $0x10

			cmp $0x00, %ah
			je vga_not_compat

		vga_compat:

			/* Use VGA registers to set the palette */

			movw $0x3c6, %dx  /* set palette mask */
			movb $0xff, %al
			outb %al, %dx

			movw $0x3c8, %dx  /* first index to set */
			xor %al, %al
			outb %al, %dx

			movw $0x3c9, %dx  /* data port */

			vga_loop:
				movb %es:2(%di), %al
				outb %al, %dx

				movb %es:1(%di), %al
				outb %al, %dx

				movb %es:(%di), %al
				outb %al, %dx

				addw $4, %di
			loop vga_loop

		vga_not_compat:

			pop %di

		vga_not_set:

		/*
		 * Store mode parameters:
		 *  eax = bpp[16] scanline[16]
		 *  ebx = width[16]  height[16]
		 *  edx = red_mask[8] red_pos[8] green_mask[8] green_pos[8]
		 *  esi = blue_mask[8] blue_pos[8]
		 *  edi = linear frame buffer
		 */

		mov VESA_MODE_BPP_OFFSET(%di), %al
		xor %ah, %ah
		shl $16, %eax
		mov VESA_MODE_SCANLINE_OFFSET(%di), %ax

		mov VESA_MODE_WIDTH_OFFSET(%di), %bx
		shl $16, %ebx
		mov VESA_MODE_HEIGHT_OFFSET(%di), %bx

		mov VESA_MODE_BLUE_MASK_OFFSET(%di), %dl
		shl $8, %edx
		mov VESA_MODE_BLUE_POS_OFFSET(%di), %dl
		mov %edx, %esi

		mov VESA_MODE_RED_MASK_OFFSET(%di), %dl
		shl $8, %edx
		mov VESA_MODE_RED_POS_OFFSET(%di), %dl

		shl $8, %edx
		mov VESA_MODE_GREEN_MASK_OFFSET(%di), %dl
		shl $8, %edx
		mov VESA_MODE_GREEN_POS_OFFSET(%di), %dl

		mov VESA_MODE_PHADDR_OFFSET(%di), %edi

		vesa_leave_real:

			mov %cr0, %ecx
			or $CR0_PE, %ecx
			mov %ecx, %cr0

			jmp vesa_leave_real2

		vesa_leave_real2:

			ljmpl $GDT_SELECTOR(KTEXT32_DES), $(vesa_init_protected - vesa_init + VESA_INIT_SEGMENT << 4)

	no_mode:

		/* No prefered mode found */

		mov $0x111, %cx
		push %di
		push %cx
		mov $VESA_GET_MODE_INFO, %ax
		int $0x10

		pop %cx
		pop %di
		cmp $VESA_OK, %al
		jnz text_mode
		jz set_mode  /* force relative jump */

	text_mode:

		/* Reset to EGA text mode (because of problems with VESA) */

		mov $0x0003, %ax
		int $0x10

		xor %eax, %eax
		xor %ebx, %ebx
		xor %edx, %edx
		xor %edi, %edi

		jz vesa_leave_real  /* force relative jump */

vga323:
#include "vga323.pal"

default_width:
	.word 0

default_height:
	.word 0

default_bpp:
	.byte 0

default_mode:
	.ascii STRING(CONFIG_BFB_MODE)
	.ascii "-"
	.asciz STRING(CONFIG_BFB_BPP)
	.fill 24

#include "vesa_ret.inc"

.align 4
e_vesa_init:
#endif
