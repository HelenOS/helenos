/*
 *  PPC linker script
 *
 */

OUTPUT_FORMAT(elf32-powerpc)
ENTRY(start)

SECTIONS {
	.boot 0x10000000: AT (0) { 
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
	}
	
	.image 0x10000000+SIZEOF(.boot): AT (SIZEOF(.boot)) SUBALIGN(4096) { 
		*(.image);
	}
}
