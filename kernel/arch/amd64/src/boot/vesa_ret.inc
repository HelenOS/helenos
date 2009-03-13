.code32
vesa_init_protected:
	movw $gdtselector(KDATA_DES), %cx
	movw %cx, %es
	movw %cx, %ds                       # kernel data + stack
	movw %cx, %ss
	
	#
	# Simics seems to remove hidden part of GS on entering user mode
	# when _visible_ part of GS does not point to user-mode segment.
	#
	
	movw $gdtselector(UDATA_DES), %cx
	movw %cx, %fs
	movw %cx, %gs
	
	movl $START_STACK, %esp             # initialize stack pointer
	
	jmpl $gdtselector(KTEXT32_DES), $vesa_meeting_point
