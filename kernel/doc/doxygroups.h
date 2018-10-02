/* Definitions of modules and its relations for generating Doxygen documentation */

/** @defgroup kernel kernel
 * @brief HelenOS kernel
 */

/** @defgroup kernel_generic_adt Data types
 * @ingroup kernel
 */

/** @defgroup main Kernel initialization
 * @ingroup others
 */

/** @defgroup kernel_generic_console Kernel console
 * @ingroup others
 */

/** @defgroup time Time management
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
 *     @defgroup abs32leproc abs32le
 *     @ingroup proc
 *     @endcond
 */

/**
 *     @cond amd64
 *     @defgroup amd64proc amd64
 *     @ingroup proc
 *     @endcond
 */

/**
 *     @cond arm32
 *     @defgroup arm32proc arm32
 *     @ingroup proc
 *     @endcond
 */

/**
 *     @cond ia32
 *     @defgroup ia32proc ia32
 *     @ingroup proc
 *     @endcond
 */

/**
 *     @cond ia64
 *     @defgroup ia64proc ia64
 *     @ingroup proc
 *     @endcond
 */

/**
 *     @cond mips32
 *     @defgroup mips32proc mips32
 *     @ingroup proc
 *     @endcond
 */

/**
 *     @cond ppc32
 *     @defgroup ppc32proc ppc32
 *     @ingroup proc
 *     @endcond
 */

/**
 *     @cond riscv64
 *     @defgroup riscv64proc riscv64
 *     @ingroup proc
 *     @endcond
 */

/**
 *     @cond sparc64
 *     @defgroup sparc64proc sparc64
 *     @ingroup proc
 *     @endcond
 */

/** @defgroup sync Synchronization
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
 *     @defgroup genarchmm genarch
 *     @ingroup mm
 */

/**
 *     @cond abs32le
 *     @defgroup abs32lemm abs32le
 *     @ingroup mm
 *     @endcond
 */

/**
 *     @cond amd64
 *     @defgroup amd64mm amd64
 *     @ingroup mm
 *     @endcond
 */

/**
 *     @cond arm32
 *     @defgroup arm32mm arm32
 *     @ingroup mm
 *     @endcond
 */

/**
 *     @cond ia32
 *     @defgroup ia32mm ia32
 *     @ingroup mm
 *     @endcond
 */

/**
 *     @cond ia64
 *     @defgroup ia64mm ia64
 *     @ingroup mm
 *     @endcond
 */

/**
 *     @cond mips32
 *     @defgroup mips32mm mips32
 *     @ingroup mm
 *     @endcond
 */

/**
 *     @cond ppc32
 *     @defgroup ppc32mm ppc32
 *     @ingroup mm
 *     @endcond
 */

/**
 *     @cond riscv64
 *     @defgroup riscv64mm riscv64
 *     @ingroup mm
 *     @endcond
 */

/**
 *     @cond sparc64
 *     @defgroup sparc64mm sparc64
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
 *     @defgroup abs32leddi abs32le
 *     @ingroup ddi
 *     @endcond
 */

/**
 *     @cond amd64
 *     @defgroup amd64ddi amd64
 *     @ingroup ddi
 *     @endcond
 */

/**
 *     @cond arm32
 *     @defgroup arm32ddi arm32
 *     @ingroup ddi
 *     @endcond
 */

/**
 *     @cond ia32
 *     @defgroup ia32ddi ia32
 *     @ingroup ddi
 *     @endcond
 */

/**
 *     @cond ia64
 *     @defgroup ia64ddi ia64
 *     @ingroup ddi
 *     @endcond
 */

/**
 *     @cond mips32
 *     @defgroup mips32ddi mips32
 *     @ingroup ddi
 *     @endcond
 */

/**
 *     @cond ppc32
 *     @defgroup ppc32ddi ppc32
 *     @ingroup ddi
 *     @endcond
 */

/**
 *     @cond riscv64
 *     @defgroup riscv64ddi riscv64
 *     @ingroup ddi
 *     @endcond
 */

/**
 *     @cond sparc64
 *     @defgroup sparc64ddi sparc64
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
 *     @defgroup abs32ledebug abs32le
 *     @ingroup debug
 *     @endcond
 */

/**
 *     @cond amd64
 *     @defgroup amd64debug ia32/amd64
 *     @ingroup debug
 *     @endcond
 */

/**
 *     @cond arm32
 *     @defgroup arm32debug arm32
 *     @ingroup debug
 *     @endcond
 */

/**
 *     @cond ia32
 *     @defgroup amd64debug ia32/amd64
 *     @ingroup debug
 *     @endcond
 */

/**
 *     @cond ia64
 *     @defgroup ia64debug ia64
 *     @ingroup debug
 *     @endcond
 */

/**
 *     @cond mips32
 *     @defgroup mips32debug mips32
 *     @ingroup debug
 *     @endcond
 */

/**
 *     @cond ppc32
 *     @defgroup ppc32debug ppc32
 *     @ingroup debug
 *     @endcond
 */

/**
 *     @cond riscv64
 *     @defgroup riscv64debug riscv64
 *     @ingroup debug
 *     @endcond
 */

/**
 *     @cond sparc64
 *     @defgroup sparc64debug sparc64
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
 *     @defgroup abs32leinterrupt abs32le
 *     @ingroup interrupt
 *     @endcond
 */

/**
 *     @cond amd64
 *     @defgroup amd64interrupt amd64
 *     @ingroup interrupt
 *     @endcond
 */

/**
 *     @cond arm32
 *     @defgroup arm32interrupt arm32
 *     @ingroup interrupt
 *     @endcond
 */

/**
 *     @cond ia32
 *     @defgroup ia32interrupt ia32
 *     @ingroup interrupt
 *     @endcond
 */

/**
 *     @cond ia64
 *     @defgroup ia64interrupt ia64
 *     @ingroup interrupt
 *     @endcond
 */

/**
 *     @cond mips32
 *     @defgroup mips32interrupt mips32
 *     @ingroup interrupt
 *     @endcond
 */

/**
 *     @cond ppc32
 *     @defgroup ppc32interrupt ppc32
 *     @ingroup interrupt
 *     @endcond
 */

/**
 *     @cond riscv64
 *     @defgroup riscv64interrupt riscv64
 *     @ingroup interrupt
 *     @endcond
 */

/**
 *     @cond sparc64
 *     @defgroup sparc64interrupt sparc64
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
 *     @defgroup genarch genarch
 *     @ingroup others
 */

/**
 *     @cond abs32le
 *     @defgroup abs32le abs32le
 *     @ingroup others
 *     @endcond
 */

/**
 *     @cond amd64
 *     @defgroup amd64 amd64
 *     @ingroup others
 *     @endcond
 */

/**
 *     @cond arm32
 *     @defgroup arm32 arm32
 *     @ingroup others
 *     @endcond
 */

/**
 *     @cond ia32
 *     @defgroup ia32 ia32
 *     @ingroup others
 *     @endcond
 */

/**
 *     @cond ia64
 *     @defgroup ia64 ia64
 *     @ingroup others
 *     @endcond
 */

/**
 *     @cond mips32
 *     @defgroup mips32 mips32
 *     @ingroup others
 *     @endcond
 */

/**
 *     @cond ppc32
 *     @defgroup ppc32 ppc32
 *     @ingroup others
 *     @endcond
 */

/**
 *     @cond riscv64
 *     @defgroup riscv64 riscv64
 *     @ingroup others
 *     @endcond
 */

/**
 *     @cond sparc64
 *     @defgroup sparc64 sparc64
 *     @ingroup others
 *     @endcond
 */
