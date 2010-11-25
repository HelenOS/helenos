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

/** @addtogroup libnet
 * @{
 */

/** @file
 * Networking interface implementation for remote modules.
 * @see net_interface.h
 */

#include <ipc/services.h>
#include <ipc/net_net.h>

#include <malloc.h>

#include <generic.h>
#include <net/modules.h>
#include <net/device.h>
#include <net_interface.h>
#include <adt/measured_strings.h>

/** Connects to the networking module.
 *
 * @return		The networking module phone on success.
 */
int net_connect_module(void)
{
	return connect_to_service(SERVICE_NETWORKING);
}

/** Frees the received settings.
 *
 * @param[in] settings	The received settings.
 * @param[in] data	The received settings data.
 * @see	net_get_device_conf_req()
 * @see net_get_conf_req()
 */
void net_free_settings(measured_string_t *settings, char *data)
{
	if (settings)
		free(settings);
	if (data)
		free(data);
}

/** Returns the global configuration.
 *
 * The configuration names are read and the appropriate settings are set
 * instead. Call net_free_settings() function to release the returned
 * configuration.
 *
 * @param[in] net_phone	The networking module phone.
 * @param[in,out] configuration The requested configuration. The names are read
 * and the appropriate settings are set instead.
 *
 * @param[in] count	The configuration entries count.
 * @param[in,out] data	The configuration and settings data.
 * @return		EOK on success.
 * @return		EINVAL if the configuration is NULL.
 * @return		EINVAL if the count is zero.
 * @return		Other error codes as defined for the
 *			generic_translate_req() function.
 */
int
net_get_conf_req(int net_phone, measured_string_t **configuration,
    size_t count, char **data)
{
	return generic_translate_req(net_phone, NET_NET_GET_DEVICE_CONF, 0, 0,
	    *configuration, count, configuration, data);
}

/** Returns the device specific configuration.
 *
 * Returns the global configuration if the device specific is not found.
 * The configuration names are read and the appropriate settings are set
 * instead. Call net_free_settings() function to release the returned
 * configuration.
 *
 * @param[in] net_phone	The networking module phone.
 * @param[in] device_id	The device identifier.
 * @param[in,out] configuration The requested device configuration. The names
 *			are read and the appropriate settings are set instead.
 * @param[in] count	The configuration entries count.
 * @param[in,out] data	The configuration and settings data.
 * @return		EOK on success.
 * @return		EINVAL if the configuration is NULL.
 * @return		EINVAL if the count is zero.
 * @return		Other error codes as defined for the
 *			generic_translate_req() function.
 */
int
net_get_device_conf_req(int net_phone, device_id_t device_id,
    measured_string_t **configuration, size_t count, char **data)
{
	return generic_translate_req(net_phone, NET_NET_GET_DEVICE_CONF,
	    device_id, 0, *configuration, count, configuration, data);
}

/** @}
 */
