.code32
vesa_init_protected:
	movw $gdtselector(KDATA_DES), %cx
	movw %cx, %es
	movw %cx, %fs
	movw %cx, %gs
	movw %cx, %ds               # kernel data + stack
	movw %cx, %ss
	
	movl $START_STACK, %esp     # initialize stack pointer
	
	jmpl $gdtselector(KTEXT_DES), $vesa_meeting_point
