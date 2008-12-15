/*
 * Copyright (C) 2005 Martin Decky
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
 
#include <ofw.h>
#include <printf.h>

typedef int (* ofw_entry_t)(ofw_args_t *args);

int ofw(ofw_args_t *args)
{
	return ((ofw_entry_t) ofw_cif)(args);
}

void write(const char *str, const int len)
{
	ofw_write(str, len);
}

int ofw_keyboard(keyboard_t *keyboard)
{
	char device_name[BUF_SIZE];
	
	if (ofw_get_property(ofw_aliases, "macio", device_name, sizeof(device_name)) <= 0)
		return false;
				
	phandle device = ofw_find_device(device_name);
	if (device == -1)
		return false;
								
	pci_reg_t macio;
	if (ofw_get_property(device, "assigned-addresses", &macio, sizeof(macio)) <= 0)
		return false;
	keyboard->addr = (void *) macio.addr.addr_lo;
	keyboard->size = macio.size_lo;

	return true;
}

int ofw_translate_failed(ofw_arg_t flag)
{
	return 0;
}
