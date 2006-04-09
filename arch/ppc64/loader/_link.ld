/*
 *  PPC linker script
 *
 */

OUTPUT_FORMAT(elf64-powerpc)
ENTRY(start)

SECTIONS {
	.boot 0x0000000010000000: AT (0) { 
		*(BOOTSTRAP);
		*(REALMODE);
		*(.text);
		
		*(.rodata);
		*(.rodata.*);
		*(.data);		/* initialized data */
		*(.sdata);
		*(.sdata2);
		*(.sbss);
		*(.bss);		/* uninitialized static variables */	
		*(COMMON); 		/* global variables */
		
		. = ALIGN(4096);
		*(.image);
	}
}
