/*
 *  PPC linker script
 *
 *  It is ELF format, but its only section looks like this:
 *  kernel text
 *  kernel data
 *
 */

OUTPUT_FORMAT(elf32-powerpc)
ENTRY(kernel_image_start)

SECTIONS {
	.image 0x80000000: AT (0x80000000) { 
		ktext_start = .;
		*(K_TEXT_START)
		*(.text);
		ktext_end = .;
		
		kdata_start = .;
		*(K_DATA_START)
		*(.rodata);
		*(.rodata.*);
		*(.data);		/* initialized data */
		*(.sdata);
		*(.sbss);
		*(.bss);		/* uninitialized static variables */	
		*(COMMON); 		/* global variables */
		kdata_end = .;
	}
	
	_hardcoded_ktext_size = ktext_end - ktext_start;
	_hardcoded_kdata_size = kdata_end - kdata_start;
	_hardcoded_load_address = 0x80000000;

}
