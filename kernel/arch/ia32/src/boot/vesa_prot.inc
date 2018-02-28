#ifdef CONFIG_FB

#define MBINFO_BIT_CMDLINE      2
#define MBINFO_OFFSET_CMDLINE   16

	/* Copy real mode VESA initialization code */

	pm_status $status_vesa_copy

	mov $vesa_init, %esi
	mov $VESA_INIT_SEGMENT << 4, %edi
	mov $e_vesa_init - vesa_init, %ecx
	rep movsb

	/* Check for multiboot command line */

	pm_status $status_multiboot_cmdline

	mov multiboot_eax, %eax
	cmp $MULTIBOOT_LOADER_MAGIC, %eax
	jne no_cmdline

	mov multiboot_ebx, %ebx
	mov (%ebx), %eax
	bt $MBINFO_BIT_CMDLINE, %eax
	jnc no_cmdline

	/* Skip the kernel path in command line */

	mov MBINFO_OFFSET_CMDLINE(%ebx), %esi

	skip_loop:
		lodsb

		cmp $0, %al
		je no_cmdline

		cmp $' ', %al
		je skip_loop_done

		jmp skip_loop
	skip_loop_done:

	space_loop:
		mov (%esi), %al

		cmp $0, %al
		je no_cmdline

		cmp $' ', %al
		jne space_loop_done

		inc %esi
		jmp space_loop
	space_loop_done:

	/* Copy at most 23 characters from command line */

	mov $VESA_INIT_SEGMENT << 4, %edi
	add $default_mode - vesa_init, %edi
	mov $23, %ecx

	cmd_loop:
		lodsb
		stosb

		cmp $0, %al
		je cmd_loop_done

		loop cmd_loop
	cmd_loop_done:

	/* Zero termination */

	xor %eax, %eax
	stosb

	no_cmdline:

	/* Jump to the real mode */

	pm_status $status_vesa_real

	mov $VESA_INIT_SEGMENT << 4, %edi
	jmpl *%edi

	vesa_meeting_point:
		/* Returned back to protected mode */

		/*
		 * Initialize Global Descriptor Table and
		 * Interrupt Descriptor Table registers
		 */
		lgdtl bootstrap_gdtr
		lidtl bootstrap_idtr

		movzx %ax, %ecx
		mov %ecx, KA2PA(bfb_scanline)

		shr $16, %eax
		mov %ax, KA2PA(bfb_bpp)

		movzx %bx, %ecx
		mov %ecx, KA2PA(bfb_height)

		shr $16, %ebx
		mov %ebx, KA2PA(bfb_width)

		mov %dl, KA2PA(bfb_green_pos)

		shr $8, %edx
		mov %dl, KA2PA(bfb_green_size)

		shr $8, %edx
		mov %dl, KA2PA(bfb_red_pos)

		shr $8, %edx
		mov %dl, KA2PA(bfb_red_size)

		mov %esi, %edx
		mov %dl, KA2PA(bfb_blue_pos)

		shr $8, %edx
		mov %dl, KA2PA(bfb_blue_size)

		mov %edi, KA2PA(bfb_addr)
#endif
