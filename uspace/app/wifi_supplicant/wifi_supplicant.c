/*
 * Copyright (c) 2015 Jan Kolarik
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

/** @addtogroup nic
 * @{
 */
/** @file WiFi device configuration utility.
 *
 */

#include <ieee80211_iface.h>

#include <inet/inetcfg.h>
#include <inet/dhcp.h>
#include <errno.h>
#include <stdio.h>
#include <loc.h>

#define NAME "wifi_supplicant"

#define enum_name(name_arr, i) ((i < 0) ? "NA" : name_arr[i])

static const char* ieee80211_security_type_strs[] = {
	"OPEN", "WEP", "WPA", "WPA2"
};

static const char* ieee80211_security_alg_strs[] = {
	"WEP40", "WEP104", "CCMP", "TKIP"
};

static const char* ieee80211_security_auth_strs[] = {
	"PSK", "8021X"
};

static void print_syntax(void)
{
	printf("syntax:\n");
	printf("\t" NAME " [<cmd> [<args...>]]\n");
	printf("\t<cmd> is:\n");
	printf("\tlist - list wifi devices in <index>: <name> format\n");
	printf("\tscan <index> [-n] - output scan results (force scan "
		"immediately)\n");
	printf("\tconnect <index> <ssid_prefix> [<password>] - connect to "
		"network\n");
	printf("\tdisconnect <index> - disconnect from network\n");
}

static char *nic_addr_format(nic_address_t *a)
{
	int rc;
	char *s;

	rc = asprintf(&s, "%02x:%02x:%02x:%02x:%02x:%02x",
	    a->address[0], a->address[1], a->address[2],
	    a->address[3], a->address[4], a->address[5]);

	if (rc < 0)
		return NULL;

	return s;
}

static int get_wifi_list(service_id_t **wifis, size_t *count)
{
	category_id_t wifi_cat;

	int rc = loc_category_get_id("ieee80211", &wifi_cat, 0);
	if (rc != EOK) {
		printf("Error resolving category 'ieee80211'.\n");
		return rc;
	}

	rc = loc_category_get_svcs(wifi_cat, wifis, count);
	if (rc != EOK) {
		printf("Error getting list of WIFIs.\n");
		return rc;
	}
	
	return EOK;
}

static async_sess_t *get_wifi_by_index(size_t i)
{
	int rc;
	size_t count;
	service_id_t *wifis = NULL;

	rc = get_wifi_list(&wifis, &count);
	if (rc != EOK) {
		printf("Error fetching wifi list.\n");
		return NULL;
	}
	
	if(i >= count) {
		printf("Invalid wifi index.\n");
		free(wifis);
		return NULL;
	}

	async_sess_t *sess = 
		loc_service_connect(EXCHANGE_SERIALIZE, wifis[i], 0);
	if (sess == NULL) {
		printf("Error connecting to service.\n");
		free(wifis);
		return NULL;
	}

	return sess;
}

static int wifi_list(void)
{
	service_id_t *wifis = NULL;
	size_t count;
	char *svc_name;
	int rc;

	rc = get_wifi_list(&wifis, &count);
	if (rc != EOK) {
		printf("Error fetching wifi list.\n");
		return EINVAL;
	}

	printf("[Index]: [Service Name]\n");
	for (size_t i = 0; i < count; i++) {
		rc = loc_service_get_name(wifis[i], &svc_name);
		if (rc != EOK) {
			printf("Error getting service name.\n");
			free(wifis);
			return rc;
		}
		
		printf("%zu: %s\n", i, svc_name);

		free(svc_name);
	}

	return EOK;
}

static int wifi_connect(uint32_t index, char *ssid_start, char *password)
{
	assert(ssid_start);
	
	async_sess_t *sess = get_wifi_by_index(index);
	if (sess == NULL) {
		printf("Specified WIFI doesn't exist or cannot connect to "
			"it.\n");
		return EINVAL;
	}
	
	int rc = ieee80211_disconnect(sess);
	if(rc != EOK) {
		if(rc == EREFUSED) {
			printf("Device is not ready yet.\n");			
		} else {
			printf("Error when disconnecting device. "
				"Error: %d\n", rc);
		}
		
		return rc;
	}
	
	ieee80211_connect(sess, ssid_start, password);
	if(rc != EOK) {
		if(rc == EREFUSED) {
			printf("Device is not ready yet.\n");			
		} else if(rc == ETIMEOUT) {
			printf("Timeout when authenticating to network.\n");
		} else if(rc == ENOENT) {
			printf("Given SSID not in scan results.\n");
		} else {
			printf("Error when connecting to network. "
				"Error: %d\n", rc);
		}
		
		return rc;
	}
	
	// TODO: Wait for DHCP address ?
	
	printf("Successfully connected to network!\n");
	
	return EOK;
}

static int wifi_disconnect(uint32_t index)
{
	async_sess_t *sess = get_wifi_by_index(index);
	if (sess == NULL) {
		printf("Specified WIFI doesn't exist or cannot connect to "
			"it.\n");
		return EINVAL;
	}
	
	int rc = ieee80211_disconnect(sess);
	if(rc != EOK) {
		if(rc == EREFUSED) {
			printf("Device is not ready yet.\n");
		} else if(rc == EINVAL) {
			printf("Not connected to any WiFi network.\n");
		} else {
			printf("Error when disconnecting from network. "
				"Error: %d\n", rc);
		}
		return rc;
	}
	
	printf("Successfully disconnected.\n");
	
	return EOK;
}

static int wifi_scan(uint32_t index, bool now)
{
	ieee80211_scan_results_t scan_results;
	
	async_sess_t *sess = get_wifi_by_index(index);
	if (sess == NULL) {
		printf("Specified WIFI doesn't exist or cannot connect to "
			"it.\n");
		return EINVAL;
	}
	
	int rc = ieee80211_get_scan_results(sess, &scan_results, now);
	if(rc != EOK) {
		if(rc == EREFUSED) {
			printf("Device is not ready yet.\n");
		} else {
			printf("Failed to fetch scan results. Error: %d\n", rc);
		}
		
		return rc;
	}
	
	if(scan_results.length == 0)
		return EOK;
	
	printf("%16.16s %17s %4s %5s %5s %7s %7s\n", 
		"SSID", "MAC", "CHAN", "TYPE", "AUTH", "UNI-ALG", "GRP-ALG");
	
	for(int i = 0; i < scan_results.length; i++) {
		ieee80211_scan_result_t result = scan_results.results[i];
		
		printf("%16.16s %17s %4d %5s %5s %7s %7s\n", 
			result.ssid, 
			nic_addr_format(&result.bssid),
			result.channel,
			enum_name(ieee80211_security_type_strs,
				result.security.type),
			enum_name(ieee80211_security_auth_strs,
				result.security.auth),
			enum_name(ieee80211_security_alg_strs,
				result.security.pair_alg),
			enum_name(ieee80211_security_alg_strs,
				result.security.group_alg)
		);
	}
	
	return EOK;
}

int main(int argc, char *argv[])
{
	int rc;
	uint32_t index;
	
	rc = inetcfg_init();
	if (rc != EOK) {
		printf(NAME ": Failed connecting to inetcfg service (%d).\n",
		    rc);
		return 1;
	}
	
	rc = dhcp_init();
	if (rc != EOK) {
		printf(NAME ": Failed connecting to dhcp service (%d).\n", rc);
		return 1;
	}
	
	if(argc == 2) {
		if(!str_cmp(argv[1], "list")) {
			return wifi_list();
		}
	} else if(argc > 2) {
		rc = str_uint32_t(argv[2], NULL, 10, false, &index);
		if(rc != EOK) {
			printf(NAME ": Invalid argument.\n");
			print_syntax();
			return EINVAL;
		}
		if(!str_cmp(argv[1], "scan")) {
			bool now = false;
			if(argc > 3)
				if(!str_cmp(argv[3], "-n"))
					now = true;
			return wifi_scan(index, now);
		} else if(!str_cmp(argv[1], "connect")) {
			char *pass = NULL;
			if(argc > 3) {
				if(argc > 4)
					pass = argv[4];
				return wifi_connect(index, argv[3], pass);
			} 
		} else if(!str_cmp(argv[1], "disconnect")) {
			return wifi_disconnect(index);
		}
	}
	
	print_syntax();
	
	return EOK;
}

/** @}
 */