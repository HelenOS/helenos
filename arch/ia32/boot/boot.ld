OUTPUT_FORMAT(binary)
SECTIONS {
    .text 0x7c00 : AT (0x0) { *(.text) }
}
