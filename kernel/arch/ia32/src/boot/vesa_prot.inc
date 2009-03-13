#ifdef CONFIG_FB
	mov $vesa_init, %esi
	mov $VESA_INIT_SEGMENT << 4, %edi
	mov $e_vesa_init - vesa_init, %ecx
	rep movsb
	
	mov $VESA_INIT_SEGMENT << 4, %edi
	jmpl *%edi
	
	vesa_meeting_point:
		mov %esi, KA2PA(vesa_ph_addr)
		mov %di, KA2PA(vesa_height)
		shr $16, %edi
		mov %di, KA2PA(vesa_width)
		mov %bx, KA2PA(vesa_scanline)
		shr $16, %ebx
		mov %bx, KA2PA(vesa_bpp)
#endif
