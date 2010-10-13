/*
 * Copyright (c) 2010 Vojtech Horky
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the distribution.
 * - The name of the author may not be used to endorse or promote products
 *   derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/** @addtogroup libusb usb
 * @{
 */
/** @file
 * @brief USB HID key codes.
 * @details
 * This is not a typical header as by default it is equal to empty file.
 * However, by cleverly defining the USB_HIDUT_KBD_KEY you can use it
 * to generate conversion tables etc.
 *
 * For example, this creates enum for known keys:
 * @code
#define USB_HIDUT_KBD_KEY(name, usage_id, l, lc, l1, l2) \
	USB_KBD_KEY_##name = usage_id,
typedef enum {
	#include <usb/hidutkbd.h>
} usb_key_code_t;
 @endcode
 *
 * Maybe, it might be better that you would place such enums into separate
 * files and create them as separate step before compiling to allow tools
 * such as Doxygen get the definitions right.
 *
 * @warning This file does not include guard to prevent multiple inclusions
 * into a single file.
 */


#ifndef USB_HIDUT_KBD_KEY
/** Declare keyboard key.
 * @param name Key name (identifier).
 * @param usage_id Key code (see Keyboard/Keypad Page (0x07) in HUT1.12.
 * @param letter Corresponding character (0 if not applicable).
 * @param letter_caps Corresponding character with Caps on.
 * @param letter_mod1 Corresponding character with modifier #1 on.
 * @param letter_mod2 Corresponding character with modifier #2 on.
 */
#define USB_HIDUT_KBD_KEY(name, usage_id, letter, letter_caps, letter_mod1, letter_mod2)

#endif

/* US alphabet letters */
USB_HIDUT_KBD_KEY(A, 0x04, 'a', 'A', 0, 0)
USB_HIDUT_KBD_KEY(B, 0x05, 'b', 'B', 0, 0)
USB_HIDUT_KBD_KEY(C, 0x06, 'c', 'C', 0, 0)
USB_HIDUT_KBD_KEY(D, 0x07, 'd', 'D', 0, 0)
USB_HIDUT_KBD_KEY(E, 0x08, 'e', 'E', 0, 0)
USB_HIDUT_KBD_KEY(F, 0x09, 'f', 'F', 0, 0)
USB_HIDUT_KBD_KEY(G, 0x0A, 'g', 'G', 0, 0)
USB_HIDUT_KBD_KEY(H, 0x0B, 'h', 'H', 0, 0)
USB_HIDUT_KBD_KEY(I, 0x0C, 'i', 'I', 0, 0)
USB_HIDUT_KBD_KEY(J, 0x0D, 'j', 'J', 0, 0)
USB_HIDUT_KBD_KEY(K, 0x0E, 'k', 'K', 0, 0)
USB_HIDUT_KBD_KEY(L, 0x0F, 'l', 'L', 0, 0)
USB_HIDUT_KBD_KEY(M, 0x10, 'm', 'M', 0, 0)
USB_HIDUT_KBD_KEY(N, 0x11, 'n', 'N', 0, 0)
USB_HIDUT_KBD_KEY(O, 0x12, 'o', 'O', 0, 0)
USB_HIDUT_KBD_KEY(P, 0x13, 'p', 'P', 0, 0)
USB_HIDUT_KBD_KEY(Q, 0x14, 'q', 'Q', 0, 0)
USB_HIDUT_KBD_KEY(R, 0x15, 'r', 'R', 0, 0)
USB_HIDUT_KBD_KEY(S, 0x16, 's', 'S', 0, 0)
USB_HIDUT_KBD_KEY(T, 0x17, 't', 'T', 0, 0)
USB_HIDUT_KBD_KEY(U, 0x18, 'u', 'U', 0, 0)
USB_HIDUT_KBD_KEY(V, 0x19, 'v', 'V', 0, 0)
USB_HIDUT_KBD_KEY(W, 0x1A, 'w', 'W', 0, 0)
USB_HIDUT_KBD_KEY(X, 0x1B, 'x', 'X', 0, 0)
USB_HIDUT_KBD_KEY(Y, 0x1C, 'y', 'Y', 0, 0)
USB_HIDUT_KBD_KEY(Z, 0x1D, 'z', 'Z', 0, 0)

/* Keyboard digits */
USB_HIDUT_KBD_KEY(1, 0x1E, '1', '!', 0, 0)
USB_HIDUT_KBD_KEY(2, 0x1F, '2', '@', 0, 0)
USB_HIDUT_KBD_KEY(3, 0x20, '3', '#', 0, 0)
USB_HIDUT_KBD_KEY(4, 0x21, '4', '$', 0, 0)
USB_HIDUT_KBD_KEY(5, 0x22, '5', '%', 0, 0)
USB_HIDUT_KBD_KEY(6, 0x23, '6', '^', 0, 0)
USB_HIDUT_KBD_KEY(7, 0x24, '7', '&', 0, 0)
USB_HIDUT_KBD_KEY(8, 0x25, '8', '*', 0, 0)
USB_HIDUT_KBD_KEY(9, 0x26, '9', '(', 0, 0)
USB_HIDUT_KBD_KEY(0, 0x27, '0', ')', 0, 0)

/* More-or-less typewriter command keys */
USB_HIDUT_KBD_KEY(ENTER, 0x28, '\n', 0, 0, 0)
USB_HIDUT_KBD_KEY(ESCAPE, 0x29, 0, 0, 0, 0)
USB_HIDUT_KBD_KEY(BACKSPACE, 0x2A, '\b', 0, 0, 0)
USB_HIDUT_KBD_KEY(TAB, 0x2B, '\t', 0, 0, 0)
USB_HIDUT_KBD_KEY(SPACE, 0x2C, ' ', 0, 0, 0)

/* Function keys */
USB_HIDUT_KBD_KEY( F1, 0x3A, 0, 0, 0, 0)
USB_HIDUT_KBD_KEY( F2, 0x3B, 0, 0, 0, 0)
USB_HIDUT_KBD_KEY( F3, 0x3C, 0, 0, 0, 0)
USB_HIDUT_KBD_KEY( F4, 0x3D, 0, 0, 0, 0)
USB_HIDUT_KBD_KEY( F5, 0x3E, 0, 0, 0, 0)
USB_HIDUT_KBD_KEY( F6, 0x3F, 0, 0, 0, 0)
USB_HIDUT_KBD_KEY( F7, 0x40, 0, 0, 0, 0)
USB_HIDUT_KBD_KEY( F8, 0x41, 0, 0, 0, 0)
USB_HIDUT_KBD_KEY( F9, 0x42, 0, 0, 0, 0)
USB_HIDUT_KBD_KEY(F10, 0x43, 0, 0, 0, 0)
USB_HIDUT_KBD_KEY(F11, 0x44, 0, 0, 0, 0)
USB_HIDUT_KBD_KEY(F12, 0x45, 0, 0, 0, 0)



/* USB_HIDUT_KBD_KEY(, 0x, '', '', 0, 0) */

/**
 * @}
 */

