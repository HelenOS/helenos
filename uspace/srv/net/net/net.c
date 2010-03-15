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
 *  Networking subsystem central module implementation.
 */

#include <async.h>
#include <ctype.h>
#include <ddi.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <ipc/ipc.h>
#include <ipc/services.h>

#include "../err.h"
#include "../messages.h"
#include "../modules.h"

#include "../structures/char_map.h"
#include "../structures/generic_char_map.h"
#include "../structures/measured_strings.h"
#include "../structures/module_map.h"
#include "../structures/packet/packet.h"

#include "../il/il_messages.h"
#include "../include/device.h"
#include "../include/netif_interface.h"
#include "../include/nil_interface.h"
#include "../include/net_interface.h"
#include "../include/ip_interface.h"

#include "net.h"
#include "net_messages.h"

/** File read buffer size.
 */
#define BUFFER_SIZE	256

/** Networking module name.
 */
#define NAME	"Networking"

/** Networking module global data.
 */
net_globals_t	net_globals;

/** Generates new system-unique device identifier.
 *  @returns The system-unique devic identifier.
 */
device_id_t generate_new_device_id(void);

/** Prints the module name.
 *  @see NAME
 */
void module_print_name(void);

/** Starts the networking module.
 *  Initializes the client connection serving function, initializes the module, registers the module service and starts the async manager, processing IPC messages in an infinite loop.
 *  @param[in] client_connection The client connection processing function. The module skeleton propagates its own one.
 *  @returns EOK on successful module termination.
 *  @returns Other error codes as defined for the net_initialize() function.
 *  @returns Other error codes as defined for the REGISTER_ME() macro function.
 */
int module_start(async_client_conn_t client_connection);

/** Returns the configured values.
 *  The network interface configuration is searched first.
 *  @param[in] netif_conf The network interface configuration setting.
 *  @param[out] configuration The found configured values.
 *  @param[in] count The desired settings count.
 *  @param[out] data The found configuration settings data.
 *  @returns EOK.
 */
int net_get_conf(measured_strings_ref netif_conf, measured_string_ref configuration, size_t count, char ** data);

/** Initializes the networking module.
 *  @param[in] client_connection The client connection processing function. The module skeleton propagates its own one.
 *  @returns EOK on success.
 *  @returns ENOMEM if there is not enough memory left.
 */
int net_initialize(async_client_conn_t client_connection);

/** \todo
 */
int parse_line(measured_strings_ref configuration, char * line);

/** Reads the networking subsystem global configuration.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the add_configuration() function.
 */
int read_configuration(void);

/** \todo
 */
int read_configuration_file(const char * directory, const char * filename, measured_strings_ref configuration);

/** Reads the network interface specific configuration.
 *  @param[in] name The network interface name.
 *  @param[in,out] netif The network interface structure.
 *  @returns EOK on success.
 *  @returns Other error codes as defined for the add_configuration() function.
 */
int read_netif_configuration(const char * name, netif_ref netif);

/** Starts the network interface according to its configuration.
 *  Registers the network interface with the subsystem modules.
 *  Starts the needed subsystem modules.
 *  @param[in] netif The network interface specific data.
 *  @returns EOK on success.
 *  @returns EINVAL if there are some settings missing.
 *  @returns ENOENT if the internet protocol module is not known.
 *  @returns Other error codes as defined for the netif_probe_req() function.
 *  @returns Other error codes as defined for the nil_device_req() function.
 *  @returns Other error codes as defined for the needed internet layer registering function.
 */
int start_device(netif_ref netif);

/** Reads the configuration and starts all network interfaces.
 *  @returns EOK on success.
 *  @returns EXDEV if there is no available system-unique device identifier.
 *  @returns EINVAL if any of the network interface names are not configured.
 *  @returns ENOMEM if there is not enough memory left.
 *  @returns Other error codes as defined for the read_configuration() function.
 *  @returns Other error codes as defined for the read_netif_configuration() function.
 *  @returns Other error codes as defined for the start_device() function.
 */
int startup(void);

GENERIC_CHAR_MAP_IMPLEMENT(measured_strings, measured_string_t)

DEVICE_MAP_IMPLEMENT(netifs, netif_t)

int add_configuration(measured_strings_ref configuration, const char * name, const char * value){
	ERROR_DECLARE;

	measured_string_ref setting;

	setting = measured_string_create_bulk(value, 0);
	if(! setting){
		return ENOMEM;
	}
	// add the configuration setting
	if(ERROR_OCCURRED(measured_strings_add(configuration, name, 0, setting))){
		free(setting);
		return ERROR_CODE;
	}
	return EOK;
}

device_id_t generate_new_device_id(void){
	return device_assign_devno();
}

void module_print_name(void){
	printf("%s", NAME);
}

int module_start(async_client_conn_t client_connection){
	ERROR_DECLARE;

	ipcarg_t phonehash;

	async_set_client_connection(client_connection);
	ERROR_PROPAGATE(pm_init());
	if(ERROR_OCCURRED(net_initialize(client_connection))
		|| ERROR_OCCURRED(REGISTER_ME(SERVICE_NETWORKING, &phonehash))){
		pm_destroy();
		return ERROR_CODE;
	}

	async_manager();

	pm_destroy();
	return EOK;
}

int net_connect_module(services_t service){
	return EOK;
}

void net_free_settings(measured_string_ref settings, char * data){
}

int net_get_conf(measured_strings_ref netif_conf, measured_string_ref configuration, size_t count, char ** data){
	measured_string_ref setting;
	size_t index;

	if(data){
		*data = NULL;
	}

	for(index = 0; index < count; ++ index){
		setting = measured_strings_find(netif_conf, configuration[index].value, 0);
		if(! setting){
			setting = measured_strings_find(&net_globals.configuration, configuration[index].value, 0);
		}
		if(setting){
			configuration[index].length = setting->length;
			configuration[index].value = setting->value;
		}else{
			configuration[index].length = 0;
			configuration[index].value = NULL;
		}
	}
	return EOK;
}

int net_get_conf_req(int net_phone, measured_string_ref * configuration, size_t count, char ** data){
	if(!(configuration && (count > 0))){
		return EINVAL;
	}

	return net_get_conf(NULL, * configuration, count, data);
}

int net_get_device_conf_req(int net_phone, device_id_t device_id, measured_string_ref * configuration, size_t count, char ** data){
	netif_ref netif;

	if(!(configuration && (count > 0))){
		return EINVAL;
	}

	netif = netifs_find(&net_globals.netifs, device_id);
	if(netif){
		return net_get_conf(&netif->configuration, * configuration, count, data);
	}else{
		return net_get_conf(NULL, * configuration, count, data);
	}
}

int net_initialize(async_client_conn_t client_connection){
	ERROR_DECLARE;

	netifs_initialize(&net_globals.netifs);
	char_map_initialize(&net_globals.netif_names);
	modules_initialize(&net_globals.modules);
	measured_strings_initialize(&net_globals.configuration);

	// TODO dynamic configuration
	ERROR_PROPAGATE(read_configuration());

	ERROR_PROPAGATE(add_module(NULL, &net_globals.modules, LO_NAME, LO_FILENAME, SERVICE_LO, 0, connect_to_service));
	ERROR_PROPAGATE(add_module(NULL, &net_globals.modules, DP8390_NAME, DP8390_FILENAME, SERVICE_DP8390, 0, connect_to_service));
	ERROR_PROPAGATE(add_module(NULL, &net_globals.modules, ETHERNET_NAME, ETHERNET_FILENAME, SERVICE_ETHERNET, 0, connect_to_service));
	ERROR_PROPAGATE(add_module(NULL, &net_globals.modules, NILDUMMY_NAME, NILDUMMY_FILENAME, SERVICE_NILDUMMY, 0, connect_to_service));

	// build specific initialization
	return net_initialize_build(client_connection);
}

int net_message(ipc_callid_t callid, ipc_call_t * call, ipc_call_t * answer, int * answer_count){
	ERROR_DECLARE;

	measured_string_ref strings;
	char * data;

	*answer_count = 0;
	switch(IPC_GET_METHOD(*call)){
		case IPC_M_PHONE_HUNGUP:
			return EOK;
		case NET_NET_GET_DEVICE_CONF:
			ERROR_PROPAGATE(measured_strings_receive(&strings, &data, IPC_GET_COUNT(call)));
			net_get_device_conf_req(0, IPC_GET_DEVICE(call), &strings, IPC_GET_COUNT(call), NULL);
			// strings should not contain received data anymore
			free(data);
			ERROR_CODE = measured_strings_reply(strings, IPC_GET_COUNT(call));
			free(strings);
			return ERROR_CODE;
		case NET_NET_GET_CONF:
			ERROR_PROPAGATE(measured_strings_receive(&strings, &data, IPC_GET_COUNT(call)));
			net_get_conf_req(0, &strings, IPC_GET_COUNT(call), NULL);
			// strings should not contain received data anymore
			free(data);
			ERROR_CODE = measured_strings_reply(strings, IPC_GET_COUNT(call));
			free(strings);
			return ERROR_CODE;
		case NET_NET_STARTUP:
			return startup();
	}
	return ENOTSUP;
}

int parse_line(measured_strings_ref configuration, char * line){
	ERROR_DECLARE;

	measured_string_ref setting;
	char * name;
	char * value;

	// from the beginning
	name = line;

	// skip comments and blank lines
	if((*name == '#') || (*name == '\0')){
		return EOK;
	}
	// skip spaces
	while(isspace(*name)){
		++ name;
	}

	// remember the name start
	value = name;
	// skip the name
	while(isalnum(*value) || (*value == '_')){
		// make uppercase
//		*value = toupper(*value);
		++ value;
	}

	if(*value == '='){
		// terminate the name
		*value = '\0';
	}else{
		// terminate the name
		*value = '\0';
		// skip until '='
		++ value;
		while((*value) && (*value != '=')){
			++ value;
		}
		// not found?
		if(*value != '='){
			return EINVAL;
		}
	}

	++ value;
	// skip spaces
	while(isspace(*value)){
		++ value;
	}
	// create a bulk measured string till the end
	setting = measured_string_create_bulk(value, 0);
	if(! setting){
		return ENOMEM;
	}

	// add the configuration setting
	if(ERROR_OCCURRED(measured_strings_add(configuration, name, 0, setting))){
		free(setting);
		return ERROR_CODE;
	}
	return EOK;
}

int read_configuration(void){
	// read the general configuration file
	return read_configuration_file(CONF_DIR, CONF_GENERAL_FILE, &net_globals.configuration);
}

int read_configuration_file(const char * directory, const char * filename, measured_strings_ref configuration){
	ERROR_DECLARE;

	size_t index = 0;
	char line[BUFFER_SIZE];
	FILE * cfg;
	int read;
	int line_number = 0;

	// construct the full filename
	printf("Reading file %s/%s\n", directory, filename);
	if(snprintf(line, BUFFER_SIZE, "%s/%s", directory, filename) > BUFFER_SIZE){
		return EOVERFLOW;
	}
	// open the file
	cfg = fopen(line, "r");
	if(! cfg){
		return ENOENT;
	}

	// read the configuration line by line
	// until an error or the end of file
	while((! ferror(cfg)) && (! feof(cfg))){
		read = fgetc(cfg);
		if((read > 0) && (read != '\n') && (read != '\r')){
			if(index >= BUFFER_SIZE){
				line[BUFFER_SIZE - 1] = '\0';
				printf("line %d too long: %s\n", line_number, line);
				// no space left in the line buffer
				return EOVERFLOW;
			}else{
				// append the character
				line[index] = (char) read;
				++ index;
			}
		}else{
			// on error or new line
			line[index] = '\0';
			++ line_number;
			if(ERROR_OCCURRED(parse_line(configuration, line))){
				printf("error on line %d: %s\n", line_number, line);
				//fclose(cfg);
				//return ERROR_CODE;
			}
			index = 0;
		}
	}
	fclose(cfg);
	return EOK;
}

int read_netif_configuration(const char * name, netif_ref netif){
	// read the netif configuration file
	return read_configuration_file(CONF_DIR, name, &netif->configuration);
}

int start_device(netif_ref netif){
	ERROR_DECLARE;

	measured_string_ref setting;
	services_t internet_service;
	int irq;
	int io;
	int mtu;

	// mandatory netif
	setting = measured_strings_find(&netif->configuration, CONF_NETIF, 0);
	netif->driver = get_running_module(&net_globals.modules, setting->value);
	if(! netif->driver){
		printf("Failed to start the network interface driver %s\n", setting->value);
		return EINVAL;
	}

	// optional network interface layer
	setting = measured_strings_find(&netif->configuration, CONF_NIL, 0);
	if(setting){
		netif->nil = get_running_module(&net_globals.modules, setting->value);
		if(! netif->nil){
			printf("Failed to start the network interface layer %s\n", setting->value);
			return EINVAL;
		}
	}else{
		netif->nil = NULL;
	}

	// mandatory internet layer
	setting = measured_strings_find(&netif->configuration, CONF_IL, 0);
	netif->il = get_running_module(&net_globals.modules, setting->value);
	if(! netif->il){
		printf("Failed to start the internet layer %s\n", setting->value);
		return EINVAL;
	}

	// hardware configuration
	setting = measured_strings_find(&netif->configuration, CONF_IRQ, 0);
	irq = setting ? strtol(setting->value, NULL, 10) : 0;
	setting = measured_strings_find(&netif->configuration, CONF_IO, 0);
	io = setting ? strtol(setting->value, NULL, 16) : 0;
	ERROR_PROPAGATE(netif_probe_req(netif->driver->phone, netif->id, irq, io));

	// network interface layer startup
	if(netif->nil){
		setting = measured_strings_find(&netif->configuration, CONF_MTU, 0);
		if(! setting){
			setting = measured_strings_find(&net_globals.configuration, CONF_MTU, 0);
		}
		mtu = setting ? strtol(setting->value, NULL, 10) : 0;
		ERROR_PROPAGATE(nil_device_req(netif->nil->phone, netif->id, mtu, netif->driver->service));
		internet_service = netif->nil->service;
	}else{
		internet_service = netif->driver->service;
	}

	// inter-network layer startup
	switch(netif->il->service){
		case SERVICE_IP:
			ERROR_PROPAGATE(ip_device_req(netif->il->phone, netif->id, internet_service));
			break;
		default:
			return ENOENT;
	}
	ERROR_PROPAGATE(netif_start_req(netif->driver->phone, netif->id));
	return EOK;
}

int startup(void){
	ERROR_DECLARE;

#ifdef CONFIG_NETIF_DP8390
	const char * conf_files[] = {"lo", "ne2k"};
#else
	const char * conf_files[] = {"lo"};
#endif

	int count = sizeof(conf_files) / sizeof(char *);
	int index;
	netif_ref netif;
	int i;
	measured_string_ref setting;

	for(i = 0; i < count; ++ i){
		netif = (netif_ref) malloc(sizeof(netif_t));
		if(! netif){
			return ENOMEM;
		}

		netif->id = generate_new_device_id();
		if(! netif->id){
			return EXDEV;
		}
		ERROR_PROPAGATE(measured_strings_initialize(&netif->configuration));

		// read configuration files
		if(ERROR_OCCURRED(read_netif_configuration(conf_files[i], netif))){
			measured_strings_destroy(&netif->configuration);
			free(netif);
			return ERROR_CODE;
		}

		// mandatory name
		setting = measured_strings_find(&netif->configuration, CONF_NAME, 0);
		if(! setting){
			printf("The name is missing\n");
			measured_strings_destroy(&netif->configuration);
			free(netif);
			return EINVAL;
		}
		netif->name = setting->value;

		// add to the netifs map
		index = netifs_add(&net_globals.netifs, netif->id, netif);
		if(index < 0){
			measured_strings_destroy(&netif->configuration);
			free(netif);
			return index;
		}

		// add to the netif names map
		if(ERROR_OCCURRED(char_map_add(&net_globals.netif_names, netif->name, 0, index))
		// start network interfaces and needed modules
			|| ERROR_OCCURRED(start_device(netif))){
			measured_strings_destroy(&netif->configuration);
			netifs_exclude_index(&net_globals.netifs, index);
			return ERROR_CODE;
		}

		// increment modules' usage
		++ netif->driver->usage;
		if(netif->nil){
			++ netif->nil->usage;
		}
		++ netif->il->usage;
		printf("New network interface started:\n\tname\t= %s\n\tid\t= %d\n\tdriver\t= %s\n\tnil\t= %s\n\til\t= %s\n", netif->name, netif->id, netif->driver->name, netif->nil ? netif->nil->name : NULL, netif->il->name);
	}
	return EOK;
}

/** @}
 */
