/*
 * SPDX-FileCopyrightText: 2013 Jan Vesely
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_arm32
 * @{
 */
/** @file
 *  @brief Security Extensions Routines
 */

#ifndef KERN_arm32_SECURITY_EXT_H_
#define KERN_arm32_SECURITY_EXT_H_

#include <arch/cp15.h>
#include <arch/regutils.h>

/** Test whether the current cpu supports security extensions.
 * return true if security extensions are supported, false otherwise.
 * @note The Processor Feature Register 1 that provides this information
 * is available only on armv7+. This function returns false on all\
 * older archs.
 */
static inline bool sec_ext_is_implemented(void)
{
#ifdef PROCESSOR_ARCH_armv7_a
	const uint32_t idpfr = ID_PFR1_read() & ID_PFR1_SEC_EXT_MASK;
	return idpfr == ID_PFR1_SEC_EXT || idpfr == ID_PFR1_SEC_EXT_RFR;
#endif
	return false;
}

/** Test whether we are running in monitor mode.
 * return true, if the current mode is Monitor mode, false otherwise.
 * @note this is safe to call even on machines that do not implement monitor
 * mode.
 */
static inline bool sec_ext_is_monitor_mode(void)
{
	return (current_status_reg_read() & MODE_MASK) == MONITOR_MODE;
}

/** Test whether we are running in a secure state.
 * return true if the current state is secure, false otherwise.
 *
 * @note: This functions will cause undef isntruction trap if we
 * are not running in the secure state.
 *
 * @note: u-boot enables non-secure access to cp 10/11, as well as some other
 * features and switches to non-secure state during boot.
 * Look for 'secureworld_exit' in arch/arm/cpu/armv7/omap3/board.c.
 */
static inline bool sec_ext_is_secure(void)
{
	return sec_ext_is_implemented() &&
	    (sec_ext_is_monitor_mode() || !(SCR_read() & SCR_NS_FLAG));
}

#endif

/** @}
 */
