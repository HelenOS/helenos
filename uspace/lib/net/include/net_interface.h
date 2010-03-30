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
 *  Networking module interface.
 *  The same interface is used for standalone remote modules as well as for bundle modules.
 *  The standalone remote modules have to be compiled with the net_remote.c source file.
 *  The bundle networking module is compiled with the net_bundle.c source file and the choosen bundle module implementation source files.
 */

#ifndef __NET_NET_INTERFACE_H__
#define __NET_NET_INTERFACE_H__

#include <ipc/services.h>

#include <net_device.h>
#include <adt/measured_strings.h>

/** @name Networking module interface
 *  This interface is used by other modules.
 */
/*@{*/

/** Returns the device specific configuration.
 *  Returns the global configuration if the device specific is not found.
 *  The configuration names are read and the appropriate settings are set instead.
 *  Call net_free_settings() function to release the returned configuration.
 *  @param[in] net_phone The networking module phone.
 *  @param[in] device_id The device identifier.
 *  @param[in,out] configuration The requested device configuration. The names are read and the appropriate settings are set instead.
 *  @param[in] count The configuration entries count.
 *  @param[in,out] data The configuration and settings data.
 *  @returns EOK on success.
 *  @returns EINVAL if the configuration is NULL.
 *  @returns EINVAL if the count is zero (0).
 *  @returns Other error codes as defined for the generic_translate_req() function.
 */
extern int net_get_device_conf_req(int net_phone, device_id_t device_id, measured_string_ref * configuration, size_t count, char ** data);

/** Returns the global configuration.
 *  The configuration names are read and the appropriate settings are set instead.
 *  Call net_free_settings() function to release the returned configuration.
 *  @param[in] net_phone The networking module phone.
 *  @param[in,out] configuration The requested configuration. The names are read and the appropriate settings are set instead.
 *  @param[in] count The configuration entries count.
 *  @param[in,out] data The configuration and settings data.
 *  @returns EOK on success.
 *  @returns EINVAL if the configuration is NULL.
 *  @returns EINVAL if the count is zero (0).
 *  @returns Other error codes as defined for the generic_translate_req() function.
 */
extern int net_get_conf_req(int net_phone, measured_string_ref * configuration, size_t count, char ** data);

/** Frees the received settings.
 *  @param[in] settings The received settings.
 *  @param[in] data The received settings data.
 *  @see net_get_device_conf_req()
 *  @see net_get_conf_req()
 */
extern void net_free_settings(measured_string_ref settings, char * data);

/** Connects to the networking module.
 *  @param service The networking module service. Ignored parameter.
 *  @returns The networking module phone on success.
 *  @returns 0 if called by the bundle module.
 */
extern int net_connect_module(services_t service);

/*@}*/

#endif

/** @}
 */
