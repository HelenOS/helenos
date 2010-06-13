OUTPUT_FORMAT("elf64-ia64-little")
ENTRY(start)

SECTIONS {
	.boot 0x4400000: AT (0x4400000) {
		*(BOOTSTRAP);
		*(.text);
		*(.sdata);
		*(.sdata2);
		*(.sbss);
		*(.rodata);
		*(.rodata.*);
		*(.data);               /* initialized data */
		_got = . ;
		*(.got .got.*);
		*(.bss);                /* uninitialized static variables */
		*(COMMON);
		*(.kernel_image);
		*(.ns_image);
		*(.loader_image);
		*(.init_image);
		*(.devmap_image);
		*(.rd_image);
		*(.vfs_image);
		*(.fat_image);
		*(.initrd_image);
	}

	/DISCARD/ : {
		*(.comment);
		*(.note*);
	}
}
