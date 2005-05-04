/*
 *  IA-64 linker script
 *
 *  It is ELF format, but its only section looks like this:
 *  kernel text
 *  kernel data
 *
 */

OUTPUT_FORMAT(elf64-ia64-little)
ENTRY(kernel_image_start)

SECTIONS {
    .image 0x0000000000001000: AT (0x0000000000001000) { 
	    ktext_start = .;
	    *(.text .stub .text.* .gnu.linkonce.t.*)
	    *(.interp)
	    *(.hash)
	    *(.dynsym)
	    *(.dynstr)
	    *(.rel.init)
	    *(.rela.init)
	    *(.rel.text .rel.text.* .rel.gnu.linkonce.t.*)
	    *(.rela.text .rela.text.* .rela.gnu.linkonce.t.*)
	    *(.rel.fini)
	    *(.rela.fini)
	    *(.rel.rodata .rel.rodata.* .rel.gnu.linkonce.r.*)
	    *(.rela.rodata .rela.rodata.* .rela.gnu.linkonce.r.*)
	    *(.rel.data .rel.data.* .rel.gnu.linkonce.d.*)
	    *(.rela.data .rela.data.* .rela.gnu.linkonce.d.*)
	    *(.rel.tdata .rel.tdata.* .rel.gnu.linkonce.td.*)
	    *(.rela.tdata .rela.tdata.* .rela.gnu.linkonce.td.*)
	    *(.rel.tbss .rel.tbss.* .rel.gnu.linkonce.tb.*)
	    *(.rela.tbss .rela.tbss.* .rela.gnu.linkonce.tb.*)
	    *(.rel.got)
	    *(.rela.got)
	    *(.rel.sdata .rel.sdata.* .rel.gnu.linkonce.s.*)
	    *(.rela.sdata .rela.sdata.* .rela.gnu.linkonce.s.*)
	    *(.rel.sbss .rel.sbss.* .rel.gnu.linkonce.sb.*)
	    *(.rela.sbss .rela.sbss.* .rela.gnu.linkonce.sb.*)
	    *(.rel.sdata2 .rel.sdata2.* .rel.gnu.linkonce.s2.*)
	    *(.rela.sdata2 .rela.sdata2.* .rela.gnu.linkonce.s2.*)
	    *(.rel.sbss2 .rel.sbss2.* .rel.gnu.linkonce.sb2.*)
	    *(.rela.sbss2 .rela.sbss2.* .rela.gnu.linkonce.sb2.*)
	    *(.rel.bss .rel.bss.* .rel.gnu.linkonce.b.*)
	    *(.rela.bss .rela.bss.* .rela.gnu.linkonce.b.*)
	    *(.rel.plt)
	    *(.rela.plt)
	    *(.rela.IA_64.pltoff)
	    *(.init)
	    *(.plt)
	    *(.fini)
	    ktext_end = .;
	    kdata_start = .;
	    *(.rodata .rodata.* .gnu.linkonce.r.*)
	    *(.rodata1)
	    *(.sdata2 .sdata2.* .gnu.linkonce.s2.*)
	    *(.sbss2 .sbss2.* .gnu.linkonce.sb2.*)
	    *(.opd)
	    *(.IA_64.unwind_info* .gnu.linkonce.ia64unwi.*)
	    *(.IA_64.unwind* .gnu.linkonce.ia64unw.*)
	    *(.eh_frame_hdr)
	    *(.preinit_array)
	    *(.init_array)
	    *(.fini_array)
	    *(.data .data.* .gnu.linkonce.d.*)
	    *(.data1)
	    *(.tdata .tdata.* .gnu.linkonce.td.*)
	    *(.tcommon)
	    *(.eh_frame)
	    *(.gcc_except_table)
	    *(.dynamic)
	    *(.jcr)
	    *(.got)
	    *(.IA_64.pltoff)
	    *(.sdata .sdata.* .gnu.linkonce.s.*)
	    *(.dynsbss)
	    *(.sbss .sbss.* .gnu.linkonce.sb.*)
	    *(.scommon)
	    *(.dynbss)
	    *(.bss .bss.* .gnu.linkonce.b.*)
	    *(COMMON)
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
        QUAD(0x0000000000001000);
    }

}
