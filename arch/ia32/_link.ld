/*
 *  i386 linker script
 *  
 *  kernel text
 *  kernel data
 *  
 */

OUTPUT_FORMAT(binary)
ENTRY(kernel_image_start)

SECTIONS {
    .image 0x8000: AT (0x8000) { 
	ktext_start = .;
	*(K_TEXT_START);
	*(.text);
	ktext_end = .;

	kdata_start = .;
	*(K_DATA_START);
	*(.data);		/* initialized data */
	*(.rodata*);		/* string literals */
	*(COMMON);		/* global variables */
	*(.bss);		/* uninitialized static variables */
	*(K_DATA_END);
	kdata_end = .;
    }

    . = ABSOLUTE(hardcoded_ktext_size);
    .patch_1 : {
        LONG(ktext_end - ktext_start);
    }

    . = ABSOLUTE(hardcoded_kdata_size);
    .patch_2 : {
        LONG(kdata_end - kdata_start);
    }

    . = ABSOLUTE(hardcoded_load_address);
    .patch_3 : {
        LONG(0x8000);
    }

}
