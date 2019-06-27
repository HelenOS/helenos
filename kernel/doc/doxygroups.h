/* Definitions of modules and its relations for generating Doxygen documentation */

/** @defgroup kernel kernel
 * @brief HelenOS kernel
 */

/** @defgroup kernel_generic_adt Data types
 * @ingroup kernel
 */

/** @defgroup kernel_generic_console Kernel console
 * @ingroup others
 */

/** @defgroup kernel_time Time management
 * @ingroup kernel
 */

/** @defgroup proc Scheduling
 * @ingroup kernel
 */

/**    @defgroup kernel_generic_proc generic
 *     @ingroup proc
 */

/**
 *     @cond abs32le
 *     @defgroup kernel_abs32le_proc abs32le
 *     @ingroup proc
 *     @endcond
 */

/**
 *     @cond amd64
 *     @defgroup kernel_amd64_proc amd64
 *     @ingroup proc
 *     @endcond
 */

/**
 *     @cond arm32
 *     @defgroup kernel_arm32_proc arm32
 *     @ingroup proc
 *     @endcond
 */

/**
 *     @cond arm64
 *     @defgroup kernel_arm64_proc arm64
 *     @ingroup proc
 *     @endcond
 */

/**
 *     @cond ia32
 *     @defgroup kernel_ia32_proc ia32
 *     @ingroup proc
 *     @endcond
 */

/**
 *     @cond ia64
 *     @defgroup kernel_ia64_proc ia64
 *     @ingroup proc
 *     @endcond
 */

/**
 *     @cond mips32
 *     @defgroup kernel_mips32_proc mips32
 *     @ingroup proc
 *     @endcond
 */

/**
 *     @cond ppc32
 *     @defgroup kernel_ppc32_proc ppc32
 *     @ingroup proc
 *     @endcond
 */

/**
 *     @cond riscv64
 *     @defgroup kernel_riscv64_proc riscv64
 *     @ingroup proc
 *     @endcond
 */

/**
 *     @cond sparc64
 *     @defgroup kernel_sparc64_proc sparc64
 *     @ingroup proc
 *     @endcond
 */

/** @defgroup kernel_sync Synchronization
 * @ingroup kernel
 */

/** @defgroup mm Memory management
 * @ingroup kernel
 */

/**
 *     @defgroup kernel_generic_mm generic
 *     @ingroup mm
 */

/**
 *     @defgroup kernel_genarch_mm genarch
 *     @ingroup mm
 */

/**
 *     @cond abs32le
 *     @defgroup kernel_abs32le_mm abs32le
 *     @ingroup mm
 *     @endcond
 */

/**
 *     @cond amd64
 *     @defgroup kernel_amd64_mm amd64
 *     @ingroup mm
 *     @endcond
 */

/**
 *     @cond arm32
 *     @defgroup kernel_arm32_mm arm32
 *     @ingroup mm
 *     @endcond
 */

/**
 *     @cond arm64
 *     @defgroup kernel_arm64_mm arm64
 *     @ingroup mm
 *     @endcond
 */

/**
 *     @cond ia32
 *     @defgroup kernel_ia32_mm ia32
 *     @ingroup mm
 *     @endcond
 */

/**
 *     @cond ia64
 *     @defgroup kernel_ia64_mm ia64
 *     @ingroup mm
 *     @endcond
 */

/**
 *     @cond mips32
 *     @defgroup kernel_mips32_mm mips32
 *     @ingroup mm
 *     @endcond
 */

/**
 *     @cond ppc32
 *     @defgroup kernel_ppc32_mm ppc32
 *     @ingroup mm
 *     @endcond
 */

/**
 *     @cond riscv64
 *     @defgroup kernel_riscv64_mm riscv64
 *     @ingroup mm
 *     @endcond
 */

/**
 *     @cond sparc64
 *     @defgroup kernel_sparc64_mm sparc64
 *     @ingroup mm
 *     @endcond
 */

/** @defgroup kernel_generic_ipc IPC
 * @ingroup kernel
 */

/** @defgroup kernel_generic_kio KIO
 * @brief Kernel character input/output facility
 * @ingroup kernel_generic_console
 */

/** @defgroup ddi Device Driver Interface
 * @ingroup kernel
 */

/**
 *     @defgroup kernel_generic_ddi generic
 *     @ingroup ddi
 */

/**
 *     @cond abs32le
 *     @defgroup kernel_abs32le_ddi abs32le
 *     @ingroup ddi
 *     @endcond
 */

/**
 *     @cond amd64
 *     @defgroup kernel_amd64_ddi amd64
 *     @ingroup ddi
 *     @endcond
 */

/**
 *     @cond arm32
 *     @defgroup kernel_arm32_ddi arm32
 *     @ingroup ddi
 *     @endcond
 */

/**
 *     @cond arm64
 *     @defgroup kernel_arm64_ddi arm64
 *     @ingroup ddi
 *     @endcond
 */

/**
 *     @cond ia32
 *     @defgroup kernel_ia32_ddi ia32
 *     @ingroup ddi
 *     @endcond
 */

/**
 *     @cond ia64
 *     @defgroup kernel_ia64_ddi ia64
 *     @ingroup ddi
 *     @endcond
 */

/**
 *     @cond mips32
 *     @defgroup kernel_mips32_ddi mips32
 *     @ingroup ddi
 *     @endcond
 */

/**
 *     @cond ppc32
 *     @defgroup kernel_ppc32_ddi ppc32
 *     @ingroup ddi
 *     @endcond
 */

/**
 *     @cond riscv64
 *     @defgroup kernel_riscv64_ddi riscv64
 *     @ingroup ddi
 *     @endcond
 */

/**
 *     @cond sparc64
 *     @defgroup kernel_sparc64_ddi sparc64
 *     @ingroup ddi
 *     @endcond
 */

/** @defgroup debug Debugging
 * @ingroup others
 */

/**
 *     @defgroup kernel_generic_debug generic
 *     @ingroup debug
 */

/**
 *     @cond abs32le
 *     @defgroup kernel_abs32le_debug abs32le
 *     @ingroup debug
 *     @endcond
 */

/**
 *     @cond amd64
 *     @defgroup kernel_amd64_debug ia32/amd64
 *     @ingroup debug
 *     @endcond
 */

/**
 *     @cond arm32
 *     @defgroup kernel_arm32_debug arm32
 *     @ingroup debug
 *     @endcond
 */

/**
 *     @cond arm64
 *     @defgroup kernel_arm64_debug arm64
 *     @ingroup debug
 *     @endcond
 */

/**
 *     @cond ia32
 *     @defgroup kernel_amd64_debug ia32/amd64
 *     @ingroup debug
 *     @endcond
 */

/**
 *     @cond ia64
 *     @defgroup kernel_ia64_debug ia64
 *     @ingroup debug
 *     @endcond
 */

/**
 *     @cond mips32
 *     @defgroup kernel_mips32_debug mips32
 *     @ingroup debug
 *     @endcond
 */

/**
 *     @cond ppc32
 *     @defgroup kernel_ppc32_debug ppc32
 *     @ingroup debug
 *     @endcond
 */

/**
 *     @cond riscv64
 *     @defgroup kernel_riscv64_debug riscv64
 *     @ingroup debug
 *     @endcond
 */

/**
 *     @cond sparc64
 *     @defgroup kernel_sparc64_debug sparc64
 *     @ingroup debug
 *     @endcond
 */

/** @defgroup interrupt Interrupt handling and dispatching
 * @ingroup kernel
 */
/**
 *     @defgroup kernel_generic_interrupt generic
 *     @ingroup interrupt
 */

/**
 *     @cond abs32le
 *     @defgroup kernel_abs32le_interrupt abs32le
 *     @ingroup interrupt
 *     @endcond
 */

/**
 *     @cond amd64
 *     @defgroup kernel_amd64_interrupt amd64
 *     @ingroup interrupt
 *     @endcond
 */

/**
 *     @cond arm32
 *     @defgroup kernel_arm32_interrupt arm32
 *     @ingroup interrupt
 *     @endcond
 */

/**
 *     @cond arm64
 *     @defgroup kernel_arm64_interrupt arm64
 *     @ingroup interrupt
 *     @endcond
 */

/**
 *     @cond ia32
 *     @defgroup kernel_ia32_interrupt ia32
 *     @ingroup interrupt
 *     @endcond
 */

/**
 *     @cond ia64
 *     @defgroup kernel_ia64_interrupt ia64
 *     @ingroup interrupt
 *     @endcond
 */

/**
 *     @cond mips32
 *     @defgroup kernel_mips32_interrupt mips32
 *     @ingroup interrupt
 *     @endcond
 */

/**
 *     @cond ppc32
 *     @defgroup kernel_ppc32_interrupt ppc32
 *     @ingroup interrupt
 *     @endcond
 */

/**
 *     @cond riscv64
 *     @defgroup kernel_riscv64_interrupt riscv64
 *     @ingroup interrupt
 *     @endcond
 */

/**
 *     @cond sparc64
 *     @defgroup kernel_sparc64_interrupt sparc64
 *     @ingroup interrupt
 *     @endcond
 */

/** @defgroup others Miscellanea
 * @ingroup kernel
 */

/**
 *     @defgroup kernel_generic generic
 *     @ingroup others
 */

/**
 *     @defgroup kernel_genarch genarch
 *     @ingroup others
 */

/**
 *     @cond abs32le
 *     @defgroup kernel_abs32le abs32le
 *     @ingroup others
 *     @endcond
 */

/**
 *     @cond amd64
 *     @defgroup kernel_amd64 amd64
 *     @ingroup others
 *     @endcond
 */

/**
 *     @cond arm32
 *     @defgroup kernel_arm32 arm32
 *     @ingroup others
 *     @endcond
 */

/**
 *     @cond arm64
 *     @defgroup kernel_arm64 arm64
 *     @ingroup others
 *     @endcond
 */

/**
 *     @cond ia32
 *     @defgroup kernel_ia32 ia32
 *     @ingroup others
 *     @endcond
 */

/**
 *     @cond ia64
 *     @defgroup kernel_ia64 ia64
 *     @ingroup others
 *     @endcond
 */

/**
 *     @cond mips32
 *     @defgroup kernel_mips32 mips32
 *     @ingroup others
 *     @endcond
 */

/**
 *     @cond ppc32
 *     @defgroup kernel_ppc32 ppc32
 *     @ingroup others
 *     @endcond
 */

/**
 *     @cond riscv64
 *     @defgroup kernel_riscv64 riscv64
 *     @ingroup others
 *     @endcond
 */

/**
 *     @cond sparc64
 *     @defgroup kernel_sparc64 sparc64
 *     @ingroup others
 *     @endcond
 */
