/*
 *  MIPS linker script
 *  
 *  kernel text
 *  kernel data
 *  
 */

OUTPUT_FORMAT(binary)
ENTRY(kernel_image_start) 

SECTIONS {
	.image 0x80000000: AT (0x80000000) {
		_gp = 0x00000000;
		
		ktext_start = .;
		*(.text);
		ktext_end = .;

		kdata_start = .;
		*(.reginfo);
		*(.rodata);
		*(.rodata.*);
		*(.rdata);
		*(.data);		/* initialized data */
		*(.lit8);
		*(.lit4);
		*(.sdata);
		*(.sbss);
		hardcoded_ktext_size = .;
		LONG(ktext_end - ktext_start);	
		hardcoded_kdata_size = .;
		LONG(kdata_end - kdata_start);
		hardcoded_load_address = .;
		LONG(0x80000000);
		*(.bss);		/* uninitialized static variables */	
		*(.scommon);
		*(COMMON); 		/* global variables */

		symbol_table = .;
		*(symtab.*);            /* Symbol table, must be LAST symbol!*/

		kdata_end = .;

	} = 0x00000000
}
