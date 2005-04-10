OUTPUT_FORMAT(binary)
ENTRY(main)
SECTIONS {
    .text 0x7c00 : AT (0x0) { *(.text) }
}
