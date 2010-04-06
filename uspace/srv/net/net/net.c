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
 * @{
 */

/** @file
 * Networking subsystem central module implementation.
 *
 */

#include <async.h>
#include <ctype.h>
#include <ddi.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <str.h>

#include <ipc/ipc.h>
#include <ipc/services.h>

#include <net_err.h>
#include <net_messages.h>
#include <net_modules.h>
#include <adt/char_map.h>
#include <adt/generic_char_map.h>
#include <adt/measured_strings.h>
#include <adt/module_map.h>
#include <packet/packet.h>
#include <il_messages.h>
#include <net_device.h>
#include <netif_interface.h>
#include <nil_interface.h>
#include <net_interface.h>
#include <ip_interface.h>
#include <net_net_messages.h>

#include "net.h"

/** Networking module name.
 *
 */
#define NAME  "net"

/** File read buffer size.
 *
 */
#define BUFFER_SIZE  256

/** Networking module global data.
 *
 */
net_globals_t net_globals;

GENERIC_CHAR_MAP_IMPLEMENT(measured_strings, measured_string_t);
DEVICE_MAP_IMPLEMENT(netifs, netif_t);

/** Add the configured setting to the configuration map.
 *
 * @param[in] configuration The configuration map.
 * @param[in] name          The setting name.
 * @param[in] value         The setting value.
 *
 * @returns EOK on success.
 * @returns ENOMEM if there is not enough memory left.
 *
 */
int add_configuration(measured_strings_ref configuration, const char *name,
    const char *value)
{
	ERROR_DECLARE;
	
	measured_string_ref setting =
	    measured_string_create_bulk(value, 0);
	
	if (!setting)
		return ENOMEM;
	
	/* Add the configuration setting */
	if (ERROR_OCCURRED(measured_strings_add(configuration, name, 0, setting))) {
		free(setting);
		return ERROR_CODE;
	}
	
	return EOK;
}

/** Generate new system-unique device identifier.
 *
 * @returns The system-unique devic identifier.
 *
 */
static device_id_t generate_new_device_id(void)
{
	return device_assign_devno();
}

static int parse_line(measured_strings_ref configuration, char *line)
{
	ERROR_DECLARE;
	
	/* From the beginning */
	char *name = line;
	
	/* Skip comments and blank lines */
	if ((*name == '#') || (*name == '\0'))
		return EOK;
	
	/* Skip spaces */
	while (isspace(*name))
		name++;
	
	/* Remember the name start */
	char *value = name;
	
	/* Skip the name */
	while (isalnum(*value) || (*value == '_'))
		value++;
	
	if (*value == '=') {
		/* Terminate the name */
		*value = '\0';
	} else {
		/* Terminate the name */
		*value = '\0';
		
		/* Skip until '=' */
		value++;
		while ((*value) && (*value != '='))
			value++;
		
		/* Not found? */
		if (*value != '=')
			return EINVAL;
	}
	
	value++;
	
	/* Skip spaces */
	while (isspace(*value))
		value++;
	
	/* Create a bulk measured string till the end */
	measured_string_ref setting =
	    measured_string_create_bulk(value, 0);
	if (!setting)
		return ENOMEM;
	
	/* Add the configuration setting */
	if (ERROR_OCCURRED(measured_strings_add(configuration, name, 0, setting))) {
		free(setting);
		return ERROR_CODE;
	}
	
	return EOK;
}

static int read_configuration_file(const char *directory, const char *filename,
    measured_strings_ref configuration)
{
	ERROR_DECLARE;
	
	printf("%s: Reading configuration file %s/%s\n", NAME, directory, filename);
	
	/* Construct the full filename */
	char line[BUFFER_SIZE];
	if (snprintf(line, BUFFER_SIZE, "%s/%s", directory, filename) > BUFFER_SIZE)
		return EOVERFLOW;
	
	/* Open the file */
	FILE *cfg = fopen(line, "r");
	if (!cfg)
		return ENOENT;
	
	/*
	 * Read the configuration line by line
	 * until an error or the end of file
	 */
	unsigned int line_number = 0;
	size_t index = 0;
	while ((!ferror(cfg)) && (!feof(cfg))) {
		int read = fgetc(cfg);
		if ((read > 0) && (read != '\n') && (read != '\r')) {
			if (index >= BUFFER_SIZE) {
				line[BUFFER_SIZE - 1] = '\0';
				fprintf(stderr, "%s: Configuration line %u too long: %s\n",
				    NAME, line_number, line);
				
				/* No space left in the line buffer */
				return EOVERFLOW;
			} else {
				/* Append the character */
				line[index] = (char) read;
				index++;
			}
		} else {
			/* On error or new line */
			line[index] = '\0';
			line_number++;
			if (ERROR_OCCURRED(parse_line(configuration, line)))
				fprintf(stderr, "%s: Configuration error on line %u: %s\n",
				    NAME, line_number, line);
			
			index = 0;
		}
	}
	
	fclose(cfg);
	return EOK;
}

/** Read the network interface specific configuration.
 *
 * @param[in]     name  The network interface name.
 * @param[in,out] netif The network interface structure.
 *
 * @returns EOK on success.
 * @returns Other error codes as defined for the add_configuration() function.
 *
 */
static int read_netif_configuration(const char *name, netif_t *netif)
{
	return read_configuration_file(CONF_DIR, name, &netif->configuration);
}

/** Read the networking subsystem global configuration.
 *
 * @returns EOK on success.
 * @returns Other error codes as defined for the add_configuration() function.
 *
 */
static int read_configuration(void)
{
	return read_configuration_file(CONF_DIR, CONF_GENERAL_FILE,
	    &net_globals.configuration);
}

/** Initialize the networking module.
 *
 * @param[in] client_connection The client connection processing
 *                              function. The module skeleton propagates
 *                              its own one.
 *
 * @returns EOK on success.
 * @returns ENOMEM if there is not enough memory left.
 *
 */
static int net_initialize(async_client_conn_t client_connection)
{
	ERROR_DECLARE;
	
	netifs_initialize(&net_globals.netifs);
	char_map_initialize(&net_globals.netif_names);
	modules_initialize(&net_globals.modules);
	measured_strings_initialize(&net_globals.configuration);
	
	// TODO: dynamic configuration
	ERROR_PROPAGATE(read_configuration());
	
	ERROR_PROPAGATE(add_module(NULL, &net_globals.modules,
	    LO_NAME, LO_FILENAME, SERVICE_LO, 0, connect_to_service));
	ERROR_PROPAGATE(add_module(NULL, &net_globals.modules,
	    DP8390_NAME, DP8390_FILENAME, SERVICE_DP8390, 0, connect_to_service));
	ERROR_PROPAGATE(add_module(NULL, &net_globals.modules,
	    ETHERNET_NAME, ETHERNET_FILENAME, SERVICE_ETHERNET, 0,
	    connect_to_service));
	ERROR_PROPAGATE(add_module(NULL, &net_globals.modules,
	    NILDUMMY_NAME, NILDUMMY_FILENAME, SERVICE_NILDUMMY, 0,
	    connect_to_service));
	
	/* Build specific initialization */
	return net_initialize_build(client_connection);
}

/** Start the networking module.
 *
 * Initializes the client connection serving function,
 * initializes the module, registers the module service
 * and starts the async manager, processing IPC messages
 * in an infinite loop.
 *
 * @param[in] client_connection The client connection
 *                              processing function. The
 *                              module skeleton propagates
 *                              its own one.
 *
 * @returns EOK on successful module termination.
 * @returns Other error codes as defined for the net_initialize() function.
 * @returns Other error codes as defined for the REGISTER_ME() macro function.
 *
 */
static int net_module_start(async_client_conn_t client_connection)
{
	ERROR_DECLARE;
	
	async_set_client_connection(client_connection);
	ERROR_PROPAGATE(pm_init());
	
	ipcarg_t phonehash;
	
	if (ERROR_OCCURRED(net_initialize(client_connection))
	    || ERROR_OCCURRED(REGISTER_ME(SERVICE_NETWORKING, &phonehash))){
		pm_destroy();
		return ERROR_CODE;
	}
	
	async_manager();
	
	pm_destroy();
	return EOK;
}

/** Return the configured values.
 *
 * The network interface configuration is searched first.
 &
 * @param[in]  netif_conf    The network interface configuration setting.
 * @param[out] configuration The found configured values.
 * @param[in]  count         The desired settings count.
 * @param[out] data          The found configuration settings data.
 *
 * @returns EOK.
 *
 */
static int net_get_conf(measured_strings_ref netif_conf,
    measured_string_ref configuration, size_t count, char **data)
{
	if (data)
		*data = NULL;
	
	size_t index;
	for (index = 0; index < count; index++) {
		measured_string_ref setting =
		    measured_strings_find(netif_conf, configuration[index].value, 0);
		if (!setting)
			setting = measured_strings_find(&net_globals.configuration,
			    configuration[index].value, 0);
		
		if (setting) {
			configuration[index].length = setting->length;
			configuration[index].value = setting->value;
		} else {
			configuration[index].length = 0;
			configuration[index].value = NULL;
		}
	}
	
	return EOK;
}

int net_get_conf_req(int net_phone, measured_string_ref *configuration,
    size_t count, char **data)
{
	if (!(configuration && (count > 0)))
		return EINVAL;
	
	return net_get_conf(NULL, *configuration, count, data);
}

int net_get_device_conf_req(int net_phone, device_id_t device_id,
    measured_string_ref *configuration, size_t count, char **data)
{
	if ((!configuration) || (count == 0))
		return EINVAL;

	netif_t *netif = netifs_find(&net_globals.netifs, device_id);
	if (netif)
		return net_get_conf(&netif->configuration, *configuration, count, data);
	else
		return net_get_conf(NULL, *configuration, count, data);
}

void net_free_settings(measured_string_ref settings, char *data)
{
}

/** Start the network interface according to its configuration.
 *
 * Register the network interface with the subsystem modules.
 * Start the needed subsystem modules.
 *
 * @param[in] netif The network interface specific data.
 *
 * @returns EOK on success.
 * @returns EINVAL if there are some settings missing.
 * @returns ENOENT if the internet protocol module is not known.
 * @returns Other error codes as defined for the netif_probe_req() function.
 * @returns Other error codes as defined for the nil_device_req() function.
 * @returns Other error codes as defined for the needed internet layer
 *          registering function.
 *
 */
static int start_device(netif_t *netif)
{
	ERROR_DECLARE;
	
	/* Mandatory netif */
	measured_string_ref setting =
	    measured_strings_find(&netif->configuration, CONF_NETIF, 0);
	
	netif->driver = get_running_module(&net_globals.modules, setting->value);
	if (!netif->driver) {
		fprintf(stderr, "%s: Failed to start network interface driver '%s'\n",
		    NAME, setting->value);
		return EINVAL;
	}
	
	/* Optional network interface layer */
	setting = measured_strings_find(&netif->configuration, CONF_NIL, 0);
	if (setting) {
		netif->nil = get_running_module(&net_globals.modules, setting->value);
		if (!netif->nil) {
			fprintf(stderr, "%s: Failed to start network interface layer '%s'\n",
			    NAME, setting->value);
			return EINVAL;
		}
	} else
		netif->nil = NULL;
	
	/* Mandatory internet layer */
	setting = measured_strings_find(&netif->configuration, CONF_IL, 0);
	netif->il = get_running_module(&net_globals.modules, setting->value);
	if (!netif->il) {
		fprintf(stderr, "%s: Failed to start internet layer '%s'\n",
		    NAME, setting->value);
		return EINVAL;
	}
	
	/* Hardware configuration */
	setting = measured_strings_find(&netif->configuration, CONF_IRQ, 0);
	int irq = setting ? strtol(setting->value, NULL, 10) : 0;
	
	setting = measured_strings_find(&netif->configuration, CONF_IO, 0);
	int io = setting ? strtol(setting->value, NULL, 16) : 0;
	
	ERROR_PROPAGATE(netif_probe_req(netif->driver->phone, netif->id, irq, io));
	
	/* Network interface layer startup */
	services_t internet_service;
	if (netif->nil) {
		setting = measured_strings_find(&netif->configuration, CONF_MTU, 0);
		if (!setting)
			setting = measured_strings_find(&net_globals.configuration,
			    CONF_MTU, 0);
		
		int mtu = setting ? strtol(setting->value, NULL, 10) : 0;
		
		ERROR_PROPAGATE(nil_device_req(netif->nil->phone, netif->id, mtu,
		    netif->driver->service));
		
		internet_service = netif->nil->service;
	} else
		internet_service = netif->driver->service;
	
	/* Inter-network layer startup */
	switch (netif->il->service) {
		case SERVICE_IP:
			ERROR_PROPAGATE(ip_device_req(netif->il->phone, netif->id,
			    internet_service));
			break;
		default:
			return ENOENT;
	}
	
	ERROR_PROPAGATE(netif_start_req(netif->driver->phone, netif->id));
	return EOK;
}

/** Read the configuration and start all network interfaces.
 *
 * @returns EOK on success.
 * @returns EXDEV if there is no available system-unique device identifier.
 * @returns EINVAL if any of the network interface names are not configured.
 * @returns ENOMEM if there is not enough memory left.
 * @returns Other error codes as defined for the read_configuration()
 *          function.
 * @returns Other error codes as defined for the read_netif_configuration()
 *          function.
 * @returns Other error codes as defined for the start_device() function.
 *
 */
static int startup(void)
{
	ERROR_DECLARE;
	
	const char *conf_files[] = {"lo", "ne2k"};
	size_t count = sizeof(conf_files) / sizeof(char *);
	
	size_t i;
	for (i = 0; i < count; i++) {
		netif_t *netif = (netif_t *) malloc(sizeof(netif_t));
		if (!netif)
			return ENOMEM;
		
		netif->id = generate_new_device_id();
		if (!netif->id)
			return EXDEV;
		
		ERROR_PROPAGATE(measured_strings_initialize(&netif->configuration));
		
		/* Read configuration files */
		if (ERROR_OCCURRED(read_netif_configuration(conf_files[i], netif))) {
			measured_strings_destroy(&netif->configuration);
			free(netif);
			return ERROR_CODE;
		}
		
		/* Mandatory name */
		measured_string_ref setting =
		    measured_strings_find(&netif->configuration, CONF_NAME, 0);
		if (!setting) {
			fprintf(stderr, "%s: Network interface name is missing\n", NAME);
			measured_strings_destroy(&netif->configuration);
			free(netif);
			return EINVAL;
		}
		netif->name = setting->value;
		
		/* Add to the netifs map */
		int index = netifs_add(&net_globals.netifs, netif->id, netif);
		if (index < 0) {
			measured_strings_destroy(&netif->configuration);
			free(netif);
			return index;
		}
		
		/*
		 * Add to the netif names map and start network interfaces
		 * and needed modules.
		 */
		if ((ERROR_OCCURRED(char_map_add(&net_globals.netif_names,
		    netif->name, 0, index))) || (ERROR_OCCURRED(start_device(netif)))) {
			measured_strings_destroy(&netif->configuration);
			netifs_exclude_index(&net_globals.netifs, index);
			return ERROR_CODE;
		}
		
		/* Increment modules' usage */
		netif->driver->usage++;
		if (netif->nil)
			netif->nil->usage++;
		netif->il->usage++;
		
		printf("%s: Network interface started (name: %s, id: %d, driver: %s, "
		    "nil: %s, il: %s)\n", NAME, netif->name, netif->id,
		    netif->driver->name,  netif->nil ? netif->nil->name : "[none]",
		    netif->il->name);
	}
	
	return EOK;
}

/** Process the networking message.
 *
 * @param[in] callid        The message identifier.
 * @param[in] call          The message parameters.
 * @param[out] answer       The message answer parameters.
 * @param[out] answer_count The last parameter for the actual answer
 *                          in the answer parameter.
 *
 * @returns EOK on success.
 * @returns ENOTSUP if the message is not known.
 *
 * @see net_interface.h
 * @see IS_NET_NET_MESSAGE()
 *
 */
int net_message(ipc_callid_t callid, ipc_call_t *call, ipc_call_t *answer,
    int *answer_count)
{
	ERROR_DECLARE;
	
	measured_string_ref strings;
	char *data;
	
	*answer_count = 0;
	switch (IPC_GET_METHOD(*call)) {
		case IPC_M_PHONE_HUNGUP:
			return EOK;
		case NET_NET_GET_DEVICE_CONF:
			ERROR_PROPAGATE(measured_strings_receive(&strings, &data,
			    IPC_GET_COUNT(call)));
			net_get_device_conf_req(0, IPC_GET_DEVICE(call), &strings,
			    IPC_GET_COUNT(call), NULL);
			
			/* Strings should not contain received data anymore */
			free(data);
			
			ERROR_CODE = measured_strings_reply(strings, IPC_GET_COUNT(call));
			free(strings);
			return ERROR_CODE;
		case NET_NET_GET_CONF:
			ERROR_PROPAGATE(measured_strings_receive(&strings, &data,
			    IPC_GET_COUNT(call)));
			net_get_conf_req(0, &strings, IPC_GET_COUNT(call), NULL);
			
			/* Strings should not contain received data anymore */
			free(data);
			
			ERROR_CODE = measured_strings_reply(strings, IPC_GET_COUNT(call));
			free(strings);
			return ERROR_CODE;
		case NET_NET_STARTUP:
			return startup();
	}
	return ENOTSUP;
}

/** Default thread for new connections.
 *
 * @param[in] iid The initial message identifier.
 * @param[in] icall The initial message call structure.
 *
 */
static void net_client_connection(ipc_callid_t iid, ipc_call_t *icall)
{
	/*
	 * Accept the connection
	 *  - Answer the first IPC_M_CONNECT_ME_TO call.
	 */
	ipc_answer_0(iid, EOK);
	
	while (true) {
		/* Clear the answer structure */
		ipc_call_t answer;
		int answer_count;
		refresh_answer(&answer, &answer_count);
		
		/* Fetch the next message */
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		/* Process the message */
		int res = net_module_message(callid, &call, &answer, &answer_count);
		
		/* End if said to either by the message or the processing result */
		if ((IPC_GET_METHOD(call) == IPC_M_PHONE_HUNGUP) || (res == EHANGUP))
			return;
		
		/* Answer the message */
		answer_call(callid, res, &answer, answer_count);
	}
}

int main(int argc, char *argv[])
{
	ERROR_DECLARE;
	
	if (ERROR_OCCURRED(net_module_start(net_client_connection))) {
		fprintf(stderr, "%s: net_module_start error %i\n", NAME, ERROR_CODE);
		return ERROR_CODE;
	}
	
	return EOK;
}

/** @}
 */
