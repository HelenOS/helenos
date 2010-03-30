/*
 * Copyright (c) 2009 Lukas Mejdrech
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

/** @addtogroup net
 *  @{
 */

/** @file
 *  Networking subsystem compilation configuration.
 */

#ifndef __NET_CONFIGURATION_H__
#define __NET_CONFIGURATION_H__

/** Activates the self test.
 */
#define NET_SELF_TEST					0

/** @name Specific self tests switches
 */
/*@{*/

/** Activates the char map self test.
 *  The NET_SELF_TEST has to be activated.
 *  @see char_map.h
 */
#define NET_SELF_TEST_CHAR_MAP			1

/** Activates the CRC computation self test.
 *  The NET_SELF_TEST has to be activated.
 *  @see crc.h
 */
#define NET_SELF_TEST_CRC				1

/** Activates the dynamic fifo self test.
 *  The NET_SELF_TEST has to be activated.
 *  @see dynamic_fifo.h
 */
#define NET_SELF_TEST_DYNAMIC_FIFO		1

/** Activates the generic char map self test.
 *  The NET_SELF_TEST has to be activated.
 *  @see generic_char_map.h
 */
#define NET_SELF_TEST_GENERIC_CHAR_MAP	1

/** Activates the generic field self test.
 *  The NET_SELF_TEST has to be activated.
 *  @see generic_field.h
 */
#define NET_SELF_TEST_GENERIC_FIELD		1

/** Activates the integral map self test.
 *  The NET_SELF_TEST has to be activated.
 *  @see int_map.h
 */
#define NET_SELF_TEST_INT_MAP			1

/** Activates the measured strings self test.
 *  The NET_SELF_TEST has to be activated.
 *  @see measured_strings.h
 */
#define NET_SELF_TEST_MEASURED_STRINGS	1

/*@}*/

#endif

/** @}
 */
