/*
 * Copyright (c) 2009 Martin Decky
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

/** @addtogroup kbd_port
 * @ingroup  kbd
 * @{
 */
/** @file
 * @brief Sun keyboard virtual port driver.
 */

#include <kbd.h>
#include <kbd_port.h>
#include <sun.h>
#include <sysinfo.h>
#include <errno.h>
#include <bool.h>

/** Sun keyboard virtual port driver.
 *
 * This is a virtual port driver which can use
 * both ns16550_port_init and z8530_port_init
 * according to the information passed from the
 * kernel. This is just a temporal hack.
 *
 */
int kbd_port_init(void)
{
	sysarg_t z8530;
	if (sysinfo_get_value("kbd.type.z8530", &z8530) != EOK)
		z8530 = false;
	
	sysarg_t ns16550;
	if (sysinfo_get_value("kbd.type.ns16550", &ns16550) != EOK)
		ns16550 = false;
	
	if (z8530) {
		if (z8530_port_init() == 0)
			return 0;
	}
	
	if (ns16550) {
		if (ns16550_port_init() == 0)
			return 0;
	}
	
	return -1;
}

void kbd_port_yield(void)
{
}

void kbd_port_reclaim(void)
{
}

void kbd_port_write(uint8_t data)
{
	(void) data;
}

/** @}
*/
