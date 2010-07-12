.code32
vesa_init_protected:
	cld
	
	/* Initialize stack pointer */
	movl $START_STACK, %esp
	
	/* Kernel data + stack */
	movw $gdtselector(KDATA_DES), %cx
	movw %cx, %es
	movw %cx, %fs
	movw %cx, %gs
	movw %cx, %ds
	movw %cx, %ss
	
	jmpl $gdtselector(KTEXT_DES), $vesa_meeting_point
