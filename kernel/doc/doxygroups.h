
/* Definitions of modules and its relations for generating Doxygen documentation */

/** @defgroup genericadt Data types
 * @ingroup kernel
 */

/** @defgroup main Kernel initialization
 * @ingroup others
 */

/** @defgroup genericconsole Kernel console
 * @ingroup others
 */

/**
  * @defgroup time Time management
  * @ingroup kernel
  */

/**
  * @defgroup proc Scheduling
  * @ingroup kernel
  */

	/** @defgroup genericproc generic
	* @ingroup proc
	*/


	/**
	 * @cond amd64
	 * @defgroup amd64proc amd64
	 * @ingroup proc
	 * @endcond
	 */

	 /**
	 * @cond arm32
	 * @defgroup arm32proc arm32
	 * @ingroup proc
	 * @endcond
	 */

	/**
	 * @cond ia32
	 * @defgroup ia32proc ia32
	 * @ingroup proc
	 * @endcond
	 */

	/**
	 * @cond ia64
	 * @defgroup ia64proc ia64
	 * @ingroup proc
	 * @endcond
	 */

	/**
	 * @cond mips32
	 * @defgroup mips32proc mips32
	 * @ingroup proc
	 * @endcond
	 */

	/**
	 * @cond ppc32
	 * @defgroup ppc32proc ppc32
	 * @ingroup proc
	 * @endcond
	 */

	/**
	 * @cond ppc64
	 * @defgroup ppc64proc ppc64
	 * @ingroup proc
	 * @endcond
	 */

	/**
	 * @cond sparc64
	 * @defgroup sparc64proc sparc64
	 * @ingroup proc
	 * @endcond
	 */


/** @defgroup sync Synchronization
 * @ingroup kernel
 */


 /** @defgroup mm Memory management
  * @ingroup kernel
  */

	/**
	 * @defgroup genericmm generic
	 * @ingroup mm
	 */

	/**
	 * @defgroup genarchmm genarch
	 * @ingroup mm
	 */

	/**
	 * @cond amd64
	 * @defgroup amd64mm amd64
	 * @ingroup mm
	 * @endcond
	 */

	/**
	 * @cond arm32
	 * @defgroup arm32mm arm32
	 * @ingroup mm
	 * @endcond
	 */

	/**
	 * @cond ia32
	 * @defgroup ia32mm ia32
	 * @ingroup mm
	 * @endcond
	 */

	/**
	 * @cond ia64
	 * @defgroup ia64mm ia64
	 * @ingroup mm
	 * @endcond
	 */

	/**
	 * @cond mips32
	 * @defgroup mips32mm mips32
	 * @ingroup mm
	 * @endcond
	 */

	/**
	 * @cond ppc32
	 * @defgroup ppc32mm ppc32
	 * @ingroup mm
	 * @endcond
	 */

	/**
	 * @cond ppc64
	 * @defgroup ppc64mm ppc64
	 * @ingroup mm
	 * @endcond
	 */

	/**
	 * @cond sparc64
	 * @defgroup sparc64mm sparc64
	 * @ingroup mm
	 * @endcond
	 */



/** @defgroup genericipc IPC
 * @ingroup kernel
 */

/** @defgroup generickio KIO
 * @brief Kernel character input/output facility
 * @ingroup genericconsole
 */


/** @defgroup ddi Device Driver Interface
 * @ingroup kernel
 */

	/** @defgroup genericddi generic
	* @ingroup ddi
 	*/

 	/**
	 * @cond amd64
	 * @defgroup amd64ddi amd64
	 * @ingroup ddi
	 * @endcond
	 */

 	/**
	 * @cond arm32
	 * @defgroup arm32ddi arm32
	 * @ingroup ddi
	 * @endcond
	 */

 	/**
	 * @cond ia32
	 * @defgroup ia32ddi ia32
	 * @ingroup ddi
	 * @endcond
	 */

 	/**
	 * @cond ia64
	 * @defgroup ia64ddi ia64
	 * @ingroup ddi
	 * @endcond
	 */

 	/**
	 * @cond mips32
	 * @defgroup mips32ddi mips32
	 * @ingroup ddi
	 * @endcond
	 */

 	/**
	 * @cond ppc32
	 * @defgroup ppc32ddi ppc32
	 * @ingroup ddi
	 * @endcond
	 */

 	/**
	 * @cond ppc64
	 * @defgroup ppc64ddi ppc64
	 * @ingroup ddi
	 * @endcond
	 */

 	/**
	 * @cond sparc64
	 * @defgroup sparc64ddi sparc64
	 * @ingroup ddi
	 * @endcond
	 */

 /** @defgroup debug Debugging
 * @ingroup others
 */

	/** @defgroup genericdebug generic
	* @ingroup debug
	*/

	/**
	 * @cond amd64
	 * @defgroup amd64debug ia32/amd64
	 * @ingroup debug
	 * @endcond
	 */

	/**
	 * @cond arm32
	 * @defgroup arm32debug arm32
	 * @ingroup debug
	 * @endcond
	 */

	/**
	 * @cond ia32
	 * @defgroup amd64debug ia32/amd64
	 * @ingroup debug
	 * @endcond
	 */

	/**
	 * @cond ia64
	 * @defgroup ia64debug ia64
	 * @ingroup debug
	 * @endcond
	 */

	/**
	 * @cond mips32
	 * @defgroup mips32debug mips32
	 * @ingroup debug
	 * @endcond
	 */

	/**
	 * @cond ppc32
	 * @defgroup ppc32debug ppc32
	 * @ingroup debug
	 * @endcond
	 */

	/**
	 * @cond ppc64
	 * @defgroup ppc64debug ppc64
	 * @ingroup debug
	 * @endcond
	 */

	/**
	 * @cond sparc64
	 * @defgroup sparc64debug sparc64
	 * @ingroup debug
	 * @endcond
	 */

 /** @defgroup interrupt Interrupt handling and dispatching
  * @ingroup kernel
  */
	/**
	 * @defgroup genericinterrupt generic
	 * @ingroup interrupt
	 */

	/**
	 * @cond amd64
	 * @defgroup amd64interrupt amd64
	 * @ingroup interrupt
	 * @endcond
	 */

	/**
	 * @cond arm32
	 * @defgroup arm32interrupt arm32
	 * @ingroup interrupt
	 * @endcond
	 */

	/**
	 * @cond ia32
	 * @defgroup ia32interrupt ia32
	 * @ingroup interrupt
	 * @endcond
	 */

	/**
	 * @cond ia64
	 * @defgroup ia64interrupt ia64
	 * @ingroup interrupt
	 * @endcond
	 */

	/**
	 * @cond mips32
	 * @defgroup mips32interrupt mips32
	 * @ingroup interrupt
	 * @endcond
	 */

	/**
	 * @cond ppc32
	 * @defgroup ppc32interrupt ppc32
	 * @ingroup interrupt
	 * @endcond
	 */

	/**
	 * @cond ppc64
	 * @defgroup ppc64interrupt ppc64
	 * @ingroup interrupt
	 * @endcond
	 */

	/**
	 * @cond sparc64
	 * @defgroup sparc64interrupt sparc64
	 * @ingroup interrupt
	 * @endcond
	 */


/** @defgroup others Miscellanea
 * @ingroup kernel
 */
	/** @defgroup generic generic
	* @ingroup others
	*/

	/** @defgroup genarch genarch
	* @ingroup others
	*/

	/**
	 * @cond amd64
	 * @defgroup amd64 amd64
	 * @ingroup others
	 * @endcond
	 */

	/**
	 * @cond arm32
	 * @defgroup arm32 arm32
	 * @ingroup others
	 * @endcond
	 */

	/**
	 * @cond ia32
	 * @defgroup ia32 ia32
	 * @ingroup others
	 * @endcond
	 */

	/**
	 * @cond ia64
	 * @defgroup ia64 ia64
	 * @ingroup others
	 * @endcond
	 */

	/**
	 * @cond mips32
	 * @defgroup mips32 mips32
	 * @ingroup others
	 * @endcond
	 */

	/**
	 * @cond ppc32
	 * @defgroup ppc32 ppc32
	 * @ingroup others
	 * @endcond
	 */

	/**
	 * @cond ppc64
	 * @defgroup ppc64 ppc64
	 * @ingroup others
	 * @endcond
	 */

	/**
	 * @cond sparc64
	 * @defgroup sparc64 sparc64
	 * @ingroup others
	 * @endcond
	 */
