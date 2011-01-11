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

#include "net.h"

#include <async.h>
#include <ctype.h>
#include <ddi.h>
#include <errno.h>
#include <malloc.h>
#include <stdio.h>
#include <str.h>

#include <ipc/ipc.h>
#include <ipc/services.h>
#include <ipc/net.h>
#include <ipc/net_net.h>
#include <ipc/il.h>
#include <ipc/nil.h>

#include <net/modules.h>
#include <net/packet.h>
#include <net/device.h>

#include <adt/char_map.h>
#include <adt/generic_char_map.h>
#include <adt/measured_strings.h>
#include <adt/module_map.h>

#include <netif_remote.h>
#include <nil_remote.h>
#include <net_interface.h>
#include <ip_interface.h>

/** Networking module name. */
#define NAME  "net"

/** File read buffer size. */
#define BUFFER_SIZE  256

/** Networking module global data. */
net_globals_t net_globals;

GENERIC_CHAR_MAP_IMPLEMENT(measured_strings, measured_string_t);
DEVICE_MAP_IMPLEMENT(netifs, netif_t);

static int startup(void);

/** Add the configured setting to the configuration map.
 *
 * @param[in] configuration The configuration map.
 * @param[in] name          The setting name.
 * @param[in] value         The setting value.
 *
 * @return EOK on success.
 * @return ENOMEM if there is not enough memory left.
 *
 */
int add_configuration(measured_strings_t *configuration, const uint8_t *name,
    const uint8_t *value)
{
	int rc;
	
	measured_string_t *setting =
	    measured_string_create_bulk(value, 0);
	if (!setting)
		return ENOMEM;
	
	/* Add the configuration setting */
	rc = measured_strings_add(configuration, name, 0, setting);
	if (rc != EOK) {
		free(setting);
		return rc;
	}
	
	return EOK;
}

/** Generate new system-unique device identifier.
 *
 * @return		The system-unique devic identifier.
 */
static device_id_t generate_new_device_id(void)
{
	return device_assign_devno();
}

static int parse_line(measured_strings_t *configuration, uint8_t *line)
{
	int rc;
	
	/* From the beginning */
	uint8_t *name = line;
	
	/* Skip comments and blank lines */
	if ((*name == '#') || (*name == '\0'))
		return EOK;
	
	/* Skip spaces */
	while (isspace(*name))
		name++;
	
	/* Remember the name start */
	uint8_t *value = name;
	
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
	measured_string_t *setting =
	    measured_string_create_bulk(value, 0);
	if (!setting)
		return ENOMEM;
	
	/* Add the configuration setting */
	rc = measured_strings_add(configuration, name, 0, setting);
	if (rc != EOK) {
		free(setting);
		return rc;
	}
	
	return EOK;
}

static int read_configuration_file(const char *directory, const char *filename,
    measured_strings_t *configuration)
{
	printf("%s: Reading configuration file %s/%s\n", NAME, directory, filename);
	
	/* Construct the full filename */
	char fname[BUFFER_SIZE];
	if (snprintf(fname, BUFFER_SIZE, "%s/%s", directory, filename) > BUFFER_SIZE)
		return EOVERFLOW;
	
	/* Open the file */
	FILE *cfg = fopen(fname, "r");
	if (!cfg)
		return ENOENT;
	
	/*
	 * Read the configuration line by line
	 * until an error or the end of file
	 */
	unsigned int line_number = 0;
	size_t index = 0;
	uint8_t line[BUFFER_SIZE];
	
	while (!ferror(cfg) && !feof(cfg)) {
		int read = fgetc(cfg);
		if ((read > 0) && (read != '\n') && (read != '\r')) {
			if (index >= BUFFER_SIZE) {
				line[BUFFER_SIZE - 1] = '\0';
				fprintf(stderr, "%s: Configuration line %u too "
				    "long: %s\n", NAME, line_number, (char *) line);
				
				/* No space left in the line buffer */
				return EOVERFLOW;
			}
			/* Append the character */
			line[index] = (uint8_t) read;
			index++;
		} else {
			/* On error or new line */
			line[index] = '\0';
			line_number++;
			if (parse_line(configuration, line) != EOK) {
				fprintf(stderr, "%s: Configuration error on "
				    "line %u: %s\n", NAME, line_number, (char *) line);
			}
			
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
 * @return EOK on success.
 * @return Other error codes as defined for the add_configuration() function.
 *
 */
static int read_netif_configuration(const char *name, netif_t *netif)
{
	return read_configuration_file(CONF_DIR, name, &netif->configuration);
}

/** Read the networking subsystem global configuration.
 *
 * @return EOK on success.
 * @return Other error codes as defined for the add_configuration() function.
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
 * @return EOK on success.
 * @return ENOMEM if there is not enough memory left.
 *
 */
static int net_initialize(async_client_conn_t client_connection)
{
	int rc;
	
	netifs_initialize(&net_globals.netifs);
	char_map_initialize(&net_globals.netif_names);
	modules_initialize(&net_globals.modules);
	measured_strings_initialize(&net_globals.configuration);
	
	/* TODO: dynamic configuration */
	rc = read_configuration();
	if (rc != EOK)
		return rc;
	
	rc = add_module(NULL, &net_globals.modules, (uint8_t *) LO_NAME,
	    (uint8_t *) LO_FILENAME, SERVICE_LO, 0, connect_to_service);
	if (rc != EOK)
		return rc;
	rc = add_module(NULL, &net_globals.modules, (uint8_t *) NE2000_NAME,
	    (uint8_t *) NE2000_FILENAME, SERVICE_NE2000, 0, connect_to_service);
	if (rc != EOK)
		return rc;
	rc = add_module(NULL, &net_globals.modules, (uint8_t *) ETHERNET_NAME,
	    (uint8_t *) ETHERNET_FILENAME, SERVICE_ETHERNET, 0, connect_to_service);
	if (rc != EOK)
		return rc;
	rc = add_module(NULL, &net_globals.modules, (uint8_t *) NILDUMMY_NAME,
	    (uint8_t *) NILDUMMY_FILENAME, SERVICE_NILDUMMY, 0, connect_to_service);
	if (rc != EOK)
		return rc;
	
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
 * @return EOK on successful module termination.
 * @return Other error codes as defined for the net_initialize() function.
 * @return Other error codes as defined for the REGISTER_ME() macro function.
 *
 */
static int net_module_start(async_client_conn_t client_connection)
{
	sysarg_t phonehash;
	int rc;
	
	async_set_client_connection(client_connection);
	rc = pm_init();
	if (rc != EOK)
		return rc;
	
	
	rc = net_initialize(client_connection);
	if (rc != EOK)
		goto out;
	
	rc = ipc_connect_to_me(PHONE_NS, SERVICE_NETWORKING, 0, 0, &phonehash);
	if (rc != EOK)
		goto out;
	
	rc = startup();
	if (rc != EOK)
		goto out;
	
	task_retval(0);
	async_manager();

out:
	pm_destroy();
	return rc;
}

/** Return the configured values.
 *
 * The network interface configuration is searched first.
 *
 * @param[in]  netif_conf    The network interface configuration setting.
 * @param[out] configuration The found configured values.
 * @param[in]  count         The desired settings count.
 * @param[out] data          The found configuration settings data.
 *
 * @return EOK.
 *
 */
static int net_get_conf(measured_strings_t *netif_conf,
    measured_string_t *configuration, size_t count, uint8_t **data)
{
	if (data)
		*data = NULL;
	
	size_t index;
	for (index = 0; index < count; index++) {
		measured_string_t *setting =
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

int net_get_conf_req(int net_phone, measured_string_t **configuration,
    size_t count, uint8_t **data)
{
	if (!configuration || (count <= 0))
		return EINVAL;
	
	return net_get_conf(NULL, *configuration, count, data);
}

int net_get_device_conf_req(int net_phone, device_id_t device_id,
    measured_string_t **configuration, size_t count, uint8_t **data)
{
	if ((!configuration) || (count == 0))
		return EINVAL;

	netif_t *netif = netifs_find(&net_globals.netifs, device_id);
	if (netif)
		return net_get_conf(&netif->configuration, *configuration, count, data);
	else
		return net_get_conf(NULL, *configuration, count, data);
}

void net_free_settings(measured_string_t *settings, uint8_t *data)
{
}

/** Start the network interface according to its configuration.
 *
 * Register the network interface with the subsystem modules.
 * Start the needed subsystem modules.
 *
 * @param[in] netif The network interface specific data.
 *
 * @return EOK on success.
 * @return EINVAL if there are some settings missing.
 * @return ENOENT if the internet protocol module is not known.
 * @return Other error codes as defined for the netif_probe_req() function.
 * @return Other error codes as defined for the nil_device_req() function.
 * @return Other error codes as defined for the needed internet layer
 *         registering function.
 *
 */
static int start_device(netif_t *netif)
{
	int rc;
	
	/* Mandatory netif */
	measured_string_t *setting =
	    measured_strings_find(&netif->configuration, (uint8_t *) CONF_NETIF, 0);
	
	netif->driver = get_running_module(&net_globals.modules, setting->value);
	if (!netif->driver) {
		fprintf(stderr, "%s: Failed to start network interface driver '%s'\n",
		    NAME, setting->value);
		return EINVAL;
	}
	
	/* Optional network interface layer */
	setting = measured_strings_find(&netif->configuration, (uint8_t *) CONF_NIL, 0);
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
	setting = measured_strings_find(&netif->configuration, (uint8_t *) CONF_IL, 0);
	netif->il = get_running_module(&net_globals.modules, setting->value);
	if (!netif->il) {
		fprintf(stderr, "%s: Failed to start internet layer '%s'\n",
		    NAME, setting->value);
		return EINVAL;
	}
	
	/* Hardware configuration */
	setting = measured_strings_find(&netif->configuration, (uint8_t *) CONF_IRQ, 0);
	int irq = setting ? strtol((char *) setting->value, NULL, 10) : 0;
	
	setting = measured_strings_find(&netif->configuration, (uint8_t *) CONF_IO, 0);
	uintptr_t io = setting ? strtol((char *) setting->value, NULL, 16) : 0;
	
	rc = netif_probe_req(netif->driver->phone, netif->id, irq, (void *) io);
	if (rc != EOK)
		return rc;
	
	/* Network interface layer startup */
	services_t internet_service;
	if (netif->nil) {
		setting = measured_strings_find(&netif->configuration, (uint8_t *) CONF_MTU, 0);
		if (!setting)
			setting = measured_strings_find(&net_globals.configuration,
			    (uint8_t *) CONF_MTU, 0);
		
		int mtu = setting ? strtol((char *) setting->value, NULL, 10) : 0;
		
		rc = nil_device_req(netif->nil->phone, netif->id, mtu,
		    netif->driver->service);
		if (rc != EOK)
			return rc;
		
		internet_service = netif->nil->service;
	} else
		internet_service = netif->driver->service;
	
	/* Inter-network layer startup */
	switch (netif->il->service) {
	case SERVICE_IP:
		rc = ip_device_req(netif->il->phone, netif->id,
		    internet_service);
		if (rc != EOK)
			return rc;
		break;
	default:
		return ENOENT;
	}
	
	return netif_start_req(netif->driver->phone, netif->id);
}

/** Read the configuration and start all network interfaces.
 *
 * @return EOK on success.
 * @return EXDEV if there is no available system-unique device identifier.
 * @return EINVAL if any of the network interface names are not configured.
 * @return ENOMEM if there is not enough memory left.
 * @return Other error codes as defined for the read_configuration()
 *         function.
 * @return Other error codes as defined for the read_netif_configuration()
 *         function.
 * @return Other error codes as defined for the start_device() function.
 *
 */
static int startup(void)
{
	const char *conf_files[] = {
		"lo",
		"ne2k"
	};
	size_t count = sizeof(conf_files) / sizeof(char *);
	int rc;
	
	size_t i;
	for (i = 0; i < count; i++) {
		netif_t *netif = (netif_t *) malloc(sizeof(netif_t));
		if (!netif)
			return ENOMEM;
		
		netif->id = generate_new_device_id();
		if (!netif->id)
			return EXDEV;
		
		rc = measured_strings_initialize(&netif->configuration);
		if (rc != EOK)
			return rc;
		
		/* Read configuration files */
		rc = read_netif_configuration(conf_files[i], netif);
		if (rc != EOK) {
			measured_strings_destroy(&netif->configuration);
			free(netif);
			return rc;
		}
		
		/* Mandatory name */
		measured_string_t *setting =
		    measured_strings_find(&netif->configuration, (uint8_t *) CONF_NAME, 0);
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
		rc = char_map_add(&net_globals.netif_names, netif->name, 0,
		    index);
		if (rc != EOK) {
			measured_strings_destroy(&netif->configuration);
			netifs_exclude_index(&net_globals.netifs, index);
			return rc;
		}
		
		rc = start_device(netif);
		if (rc != EOK) {
			measured_strings_destroy(&netif->configuration);
			netifs_exclude_index(&net_globals.netifs, index);
			return rc;
		}
		
		/* Increment modules' usage */
		netif->driver->usage++;
		if (netif->nil)
			netif->nil->usage++;
		netif->il->usage++;
		
		printf("%s: Network interface started (name: %s, id: %d, driver: %s, "
		    "nil: %s, il: %s)\n", NAME, netif->name, netif->id,
		    netif->driver->name, netif->nil ? (char *) netif->nil->name : "[none]",
		    netif->il->name);
	}
	
	return EOK;
}

/** Process the networking message.
 *
 * @param[in]  callid       The message identifier.
 * @param[in]  call         The message parameters.
 * @param[out] answer       The message answer parameters.
 * @param[out] answer_count The last parameter for the actual answer
 *                          in the answer parameter.
 *
 * @return EOK on success.
 * @return ENOTSUP if the message is not known.
 *
 * @see net_interface.h
 * @see IS_NET_NET_MESSAGE()
 *
 */
int net_message(ipc_callid_t callid, ipc_call_t *call, ipc_call_t *answer,
    size_t *answer_count)
{
	measured_string_t *strings;
	uint8_t *data;
	int rc;
	
	*answer_count = 0;
	switch (IPC_GET_IMETHOD(*call)) {
	case IPC_M_PHONE_HUNGUP:
		return EOK;
	case NET_NET_GET_DEVICE_CONF:
		rc = measured_strings_receive(&strings, &data,
		    IPC_GET_COUNT(*call));
		if (rc != EOK)
			return rc;
		net_get_device_conf_req(0, IPC_GET_DEVICE(*call), &strings,
		    IPC_GET_COUNT(*call), NULL);
		
		/* Strings should not contain received data anymore */
		free(data);
		
		rc = measured_strings_reply(strings, IPC_GET_COUNT(*call));
		free(strings);
		return rc;
	case NET_NET_GET_CONF:
		rc = measured_strings_receive(&strings, &data,
		    IPC_GET_COUNT(*call));
		if (rc != EOK)
			return rc;
		net_get_conf_req(0, &strings, IPC_GET_COUNT(*call), NULL);
		
		/* Strings should not contain received data anymore */
		free(data);
		
		rc = measured_strings_reply(strings, IPC_GET_COUNT(*call));
		free(strings);
		return rc;
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
		size_t answer_count;
		refresh_answer(&answer, &answer_count);
		
		/* Fetch the next message */
		ipc_call_t call;
		ipc_callid_t callid = async_get_call(&call);
		
		/* Process the message */
		int res = net_module_message(callid, &call, &answer, &answer_count);
		
		/* End if told to either by the message or the processing result */
		if ((IPC_GET_IMETHOD(call) == IPC_M_PHONE_HUNGUP) || (res == EHANGUP))
			return;
		
		/* Answer the message */
		answer_call(callid, res, &answer, answer_count);
	}
}

int main(int argc, char *argv[])
{
	int rc;
	
	rc = net_module_start(net_client_connection);
	if (rc != EOK) {
		fprintf(stderr, "%s: net_module_start error %i\n", NAME, rc);
		return rc;
	}
	
	return EOK;
}

/** @}
 */
