/*
 *  MIPS linker script
 *  
 *  kernel text
 *  kernel data
 *  
 */

/* OUTPUT_FORMAT(ecoff-littlemips) */
OUTPUT_FORMAT(binary)
SECTIONS {
	.image 0x0: AT (0) {
		*(.text);
		*(.reginfo);
	} 
}
