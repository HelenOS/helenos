/** AMD64 linker script
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
		*(K_TEXT_START_2);
		unmapped_ktext_end = .;
		unmapped_kdata_start = .;
		*(K_DATA_START);
		unmapped_kdata_end = .;
	}

	.mapped (0xffffffff80000000+SIZEOF(.unmapped)+0x8000) : AT (0x8000+SIZEOF(.unmapped)) {
		ktext_start = .;
		*(BOOT_DATA);
		*(.text);
		ktext_end = .;

		kdata_start = .;
		*(.data);		/* initialized data */
		*(.rodata*);		/* string literals */
		hardcoded_load_address = .;
		QUAD(0xffffffff80008000);
		hardcoded_ktext_size = .;
		QUAD(ktext_end - ktext_start + (unmapped_ktext_end - unmapped_ktext_start));
		hardcoded_kdata_size = .;
		QUAD(kdata_end - kdata_start + (unmapped_kdata_end - unmapped_kdata_start));
		*(COMMON);		/* global variables */



		*(.eh_frame);
		*(.bss);		/* uninitialized static variables */
		*(K_DATA_END);

		symbol_table = .;
		*(symtab.*);            /* Symbol table, must be LAST symbol!*/

		kdata_end = .;
	}
	_map_address = 0xffffffff80000000;
	e820table_boot = e820table - _map_address;
	e820counter_boot = e820counter - _map_address;
}
