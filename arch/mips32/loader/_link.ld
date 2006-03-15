/*
 *  MIPS linker script
 *
 */

OUTPUT_FORMAT(binary)
ENTRY(start)

SECTIONS {
	.image 0xbfc00000: AT (0) { 
		*(BOOTSTRAP)
		*(.text);
		
		*(.rodata);
		*(.rodata.*);
		*(.data);		/* initialized data */
		*(.sdata);
		*(.sdata2);
		*(.sbss);
		*(.bss);		/* uninitialized static variables */	
		*(COMMON); 		/* global variables */
		*(.reginfo);
	}
}
