/** IA-32 linker script
 *  
 * umapped section:
 * 	kernel text
 * 	kernel data
 * mapped section:
 *	kernel text
 *	kernel data 
 */

OUTPUT_FORMAT(binary)
ENTRY(kernel_image_start)

SECTIONS {
    .unmapped 0x8000: AT (0x8000) {
	unmapped_ktext_start = .;
	*(K_TEXT_START);
	unmapped_ktext_end = .;
	unmapped_kdata_start = .;
	*(K_DATA_START);
	unmapped_kdata_end = .;
    }
    
    .mapped (0x80000000+SIZEOF(.unmapped)+0x8000) : AT (0x8000+SIZEOF(.unmapped)) { 
	ktext_start = .;
	*(.text);
	ktext_end = .;

	kdata_start = .;
	*(.data);		/* initialized data */
	*(.rodata*);		/* string literals */
	*(COMMON);		/* global variables */
	*(.bss);		/* uninitialized static variables */
	*(K_DATA_END);
	kdata_end = .;
    }

    _hardcoded_ktext_size = ktext_end - ktext_start + (unmapped_ktext_end - unmapped_ktext_start);
    _hardcoded_kdata_size = kdata_end - kdata_start + (unmapped_kdata_end - unmapped_kdata_start);
    _hardcoded_load_address = 0x80008000;

}
