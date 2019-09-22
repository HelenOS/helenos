#!/bin/sh

# User-controllable options
grub_modinfo_target_cpu=arm64
grub_modinfo_platform=efi
grub_disk_cache_stats=0
grub_boot_time_stats=0
grub_have_font_source=1

# Autodetected config
grub_have_asm_uscore=0
grub_bss_start_symbol=""
grub_end_symbol=""

# Build environment
grub_target_cc='aarch64-linux-gnu-gcc'
grub_target_cc_version='aarch64-linux-gnu-gcc (Linaro GCC 6.3-2017.02) 6.3.1 20170109'
grub_target_cflags=' -Os -Wall -W -Wshadow -Wpointer-arith -Wundef -Wchar-subscripts -Wcomment -Wdeprecated-declarations -Wdisabled-optimization -Wdiv-by-zero -Wfloat-equal -Wformat-extra-args -Wformat-security -Wformat-y2k -Wimplicit -Wimplicit-function-declaration -Wimplicit-int -Wmain -Wmissing-braces -Wmissing-format-attribute -Wmultichar -Wparentheses -Wreturn-type -Wsequence-point -Wshadow -Wsign-compare -Wswitch -Wtrigraphs -Wunknown-pragmas -Wunused -Wunused-function -Wunused-label -Wunused-parameter -Wunused-value  -Wunused-variable -Wwrite-strings -Wnested-externs -Wstrict-prototypes -g -Wredundant-decls -Wmissing-prototypes -Wmissing-declarations -Wcast-align  -Wextra -Wattributes -Wendif-labels -Winit-self -Wint-to-pointer-cast -Winvalid-pch -Wmissing-field-initializers -Wnonnull -Woverflow -Wvla -Wpointer-to-int-cast -Wstrict-aliasing -Wvariadic-macros -Wvolatile-register-var -Wpointer-sign -Wmissing-include-dirs -Wmissing-prototypes -Wmissing-declarations -Wformat=2 -freg-struct-return -mgeneral-regs-only -fno-dwarf2-cfi-asm -fno-asynchronous-unwind-tables -fno-unwind-tables -Qn -fno-stack-protector -Wtrampolines'
grub_target_cppflags=' -Wall -W  -DGRUB_MACHINE_EFI=1 -DGRUB_MACHINE=ARM64_EFI -nostdinc -isystem /home/setup/opt/gcc-linaro-6.3.1-2017.02-x86_64_aarch64-linux-gnu/bin/../lib/gcc/aarch64-linux-gnu/6.3.1/include -I$(top_srcdir)/include -I$(top_builddir)/include'
grub_target_ccasflags=' -g  -mgeneral-regs-only'
grub_target_ldflags=' -Wl,--build-id=none'
grub_cflags=''
grub_cppflags=' -D_FILE_OFFSET_BITS=64'
grub_ccasflags=''
grub_ldflags=''
grub_target_strip='aarch64-linux-gnu-strip'
grub_target_nm='aarch64-linux-gnu-nm'
grub_target_ranlib='aarch64-linux-gnu-ranlib'
grub_target_objconf=''
grub_target_obj2elf=''
grub_target_img_base_ldopt='-Wl,-Ttext'
grub_target_img_ldflags='@TARGET_IMG_BASE_LDFLAGS@'

# Version
grub_version="2.02~beta2"
grub_package="grub"
grub_package_string="GRUB 2.02~beta2"
grub_package_version="2.02~beta2"
grub_package_name="GRUB"
grub_package_bugreport="bug-grub@gnu.org"
