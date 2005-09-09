/*
 *  MIPS linker script
 *  
 *  kernel text
 *  kernel data
 *  
 */

/* OUTPUT_FORMAT(ecoff-littlemips) */
OUTPUT_FORMAT(binary)
/* OUTPUT_FORMAT(elf32-little) */

ENTRY(kernel_image_start) 

SECTIONS {
	.image 0x80000000: AT (0) {
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
		*(.comment); 
		*(.pdr); 

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

	} 
}
