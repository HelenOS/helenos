/*
 *  MIPS linker script
 *  
 *  kernel text
 *  kernel data
 *  
 */

OUTPUT_FORMAT(binary)
ENTRY(kernel_image_start) 

SECTIONS {
	.image 0x80000000: AT (0x80000000) { 
		ktext_start = .;
		*(.text);
		ktext_end = .;

		kdata_start = .;
		*(.rdata);
		*(.data);		/* initialized data */
		*(.lit8);
		*(.lit4);
		*(.sdata);
		*(.sbss);
		*(.bss);		/* uninitialized static variables */	
		*(.scommon);
		*(COMMON); 		/* global variables */
		kdata_end = .;

	} = 0x00000000

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
		LONG(0x80000000);
	}

}
