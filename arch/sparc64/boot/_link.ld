/*
 *  boot[sparc64] linker script
 *
 */

OUTPUT_FORMAT(a.out-sunos-big)
ENTRY(start)

SECTIONS {
	.text 0x8000: AT (0x8000) { 
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
