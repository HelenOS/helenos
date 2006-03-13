/*
 *  PPC linker script
 *
 */

OUTPUT_FORMAT(elf32-powerpc)
ENTRY(start)

SECTIONS {
	.image 0x10000000: AT (0) { 
		*(BOOTSTRAP)
		*(.text);
		
		*(.rodata);
		*(.rodata.*);
		*(.data);		/* initialized data */
		*(.sdata);
		*(.sdata2);
		*(.sbss);
		*(.bss);		/* uninitialized static variables */	
	}
}
