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
#include <async.h>
#include <devman.h>
#include <generic.h>
#include <net/modules.h>
#include <net/device.h>
#include <net_interface.h>
#include <adt/measured_strings.h>

/** Connect to the networking module.
 *
 * @return Networking module session on success.
 *
 */
async_sess_t *net_connect_module(void)
{
	return connect_to_service(SERVICE_NETWORKING);
}

/** Free the received settings.
 *
 * @param[in] settings Received settings.
 * @param[in] data     Received settings data.
 *
 * @see net_get_device_conf_req()
 * @see net_get_conf_req()
 *
 */
void net_free_settings(measured_string_t *settings, uint8_t *data)
{
	if (settings)
		free(settings);
	if (data)
		free(data);
}

/** Return the global configuration.
 *
 * The configuration names are read and the appropriate settings are set
 * instead. Call net_free_settings() function to release the returned
 * configuration.
 *
 * @param[in]     sess          Networking module session.
 * @param[in,out] configuration Requested configuration. The names are
 *                              read and the appropriate settings are set
 *                              instead.
 *
 * @param[in]     count         Configuration entries count.
 * @param[in,out] data          Configuration and settings data.
 *
 * @return EOK on success.
 * @return EINVAL if the configuration is NULL.
 * @return EINVAL if the count is zero.
 * @return Other error codes as defined for the
 *         generic_translate_req() function.
 *
 */
int net_get_conf_req(async_sess_t *sess, measured_string_t **configuration,
    size_t count, uint8_t **data)
{
	return generic_translate_req(sess, NET_NET_GET_CONF, 0, 0,
	    *configuration, count, configuration, data);
}

/** Return the device specific configuration.
 *
 * Return the global configuration if the device specific is not found.
 * The configuration names are read and the appropriate settings are set
 * instead. Call net_free_settings() function to release the returned
 * configuration.
 *
 * @param[in]     sess          The networking module session.
 * @param[in]     device_id     Device identifier.
 * @param[in,out] configuration Requested device configuration. The names
 *                              are read and the appropriate settings are
 *                              set instead.
 * @param[in]     count         Configuration entries count.
 * @param[in,out] data          Configuration and settings data.
 *
 * @return EOK on success.
 * @return EINVAL if the configuration is NULL.
 * @return EINVAL if the count is zero.
 * @return Other error codes as defined for the
 *         generic_translate_req() function.
 *
 */
int net_get_device_conf_req(async_sess_t *sess, nic_device_id_t device_id,
    measured_string_t **configuration, size_t count, uint8_t **data)
{
	return generic_translate_req(sess, NET_NET_GET_DEVICE_CONF,
	    device_id, 0, *configuration, count, configuration, data);
}

int net_get_devices_req(async_sess_t *sess, measured_string_t **devices,
    size_t *count, uint8_t **data)
{
	if ((!devices) || (!count))
		return EBADMEM;
	
	async_exch_t *exch = async_exchange_begin(sess);
	
	int rc = async_req_0_1(exch, NET_NET_GET_DEVICES_COUNT, count);
	if (rc != EOK) {
		async_exchange_end(exch);
		return rc;
	}
	
	if (*count == 0) {
		async_exchange_end(exch);
		*data = NULL;
		return EOK;
	}
	
	aid_t message_id = async_send_0(exch, NET_NET_GET_DEVICES, NULL);
	rc = measured_strings_return(exch, devices, data, *count);
	
	async_exchange_end(exch);
	
	sysarg_t result;
	async_wait_for(message_id, &result);
	
	if ((rc == EOK) && (result != EOK)) {
		free(*devices);
		free(*data);
	}
	
	return (int) result;
}

int net_driver_ready(async_sess_t *sess, devman_handle_t handle)
{
	async_exch_t *exch = async_exchange_begin(sess);
	int rc = async_req_1_0(exch, NET_NET_DRIVER_READY, handle);
	async_exchange_end(exch);
	
	return rc;
}

/** @}
 */
