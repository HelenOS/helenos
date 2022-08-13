/*
 * SPDX-FileCopyrightText: 2005 Martin Decky
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/** @addtogroup kernel_ppc32
 * @{
 */
/** @file
 */

#ifndef KERN_ppc32_MSR_H_
#define KERN_ppc32_MSR_H_

/* MSR bits */
#define MSR_DR  (1 << 4)
#define MSR_IR  (1 << 5)
#define MSR_FE1 (1 << 8)
#define MSR_FE0 (1 << 11)
#define MSR_FP  (1 << 13)
#define MSR_PR  (1 << 14)
#define MSR_EE  (1 << 15)

/* HID0 bits */
#define HID0_STEN  (1 << 24)
#define HID0_ICE   (1 << 15)
#define HID0_DCE   (1 << 14)
#define HID0_ICFI  (1 << 11)
#define HID0_DCI   (1 << 10)

#endif

/** @}
 */
