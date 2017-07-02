#include <espressif/esp_common.h>
#include <esp8266.h>
#include <esp/uart.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <stdlib.h>
#include <string.h>
#include <dhcpserver.h>
#include <limits.h>
#include "queue.h"
#include "WifiManager.h"
#include "web_config.h"
#include "jsmn.h"
#include "fileutils.h"

char parseJSON(char *json_data);
void post_recv_callback(char *json_data);

char *keys[2] = { "ssid", "password" };
struct sdk_station_config config = {
	.ssid = "",
	.password = "",
};

void post_recv_callback(char *json_data);

bool
hasWifiConfig()
{
	// check if wifi config file exists
	if (fileExists(FILE_WIFI_CONF)) {
#ifdef WM_DBG
		printf("Config file exists.\n");
#endif
		char *readbuf = (char *) malloc(500 * sizeof(char));
		int bytes = read_file(FILE_WIFI_CONF, readbuf);
#ifdef WM_DBG
		printf("Configuration loaded: %s\n", readbuf);
#endif
		if (bytes > 0 && parseJSON(readbuf) != ERROR_OK)
		{
			free(readbuf);
			return false;
		}
		return true;
	}
#ifdef WM_DBG
	printf("Wifi config not found.\n");
#endif

	return false;
}

uint8_t
check_wifi_connection()
{
	uint8_t status = UCHAR_MAX;
	uint8_t timeout = 0;
#ifdef WM_DBG
		printf("Checking connection.");
#endif
	while (status != STATION_GOT_IP && timeout < CONNECTION_TIMEOUT) {
		status = sdk_wifi_station_get_connect_status();

		switch (status) {
		case STATION_WRONG_PASSWORD:
#ifdef WM_DBG
			printf("WiFi: wrong password.\r\n");
#endif
			sdk_wifi_station_disconnect();
			break;
		case STATION_NO_AP_FOUND:
			sdk_wifi_station_disconnect();
#ifdef WM_DBG
			printf("WiFi: AP not found.\r\n");
#endif
			break;
		case STATION_CONNECT_FAIL:
#ifdef WM_DBG
			printf("WiFi: connection failed.\r\n");
#endif
			break;
		case STATION_GOT_IP:
#ifdef WM_DBG
			printf("WiFi: connected.\r\n");
#endif
			break;
		case STATION_IDLE:
#ifdef WM_DBG
			printf("WiFi: idle.\r\n");
#endif
			break;
		}
		timeout ++;
		vTaskDelay(1000 / portTICK_PERIOD_MS);
#ifdef WM_DBG
		printf(".");
#endif
	}
#ifdef WM_DBG
	printf("\n");
#endif
	return status;
}

void
configurationDone(QueueHandle_t * queue)
{
#ifdef WM_DBG
	printf("Wifi client connected. Notifying main program.\n");
#endif
	xQueueSend(*queue, NULL, 0);
	vTaskDelete(NULL);
}

bool
init_wifi_client()
{
	/* required to call wifi_set_opmode before station_set_config */
	sdk_wifi_set_opmode(STATION_MODE);
	sdk_wifi_station_set_config(&config);
	sdk_wifi_station_connect();
	return check_wifi_connection() == STATION_GOT_IP;
}

void
init_wifi_ap()
{
	sdk_wifi_set_opmode(SOFTAP_MODE);
	struct ip_info  ap_ip;
	IP4_ADDR(&ap_ip.ip, 192, 168, 0, 1);
	IP4_ADDR(&ap_ip.gw, 0, 0, 0, 0);
	IP4_ADDR(&ap_ip.netmask, 255, 255, 0, 0);
	sdk_wifi_set_ip_info(1, &ap_ip);
	struct sdk_softap_config ap_config = {
		.ssid = AP_SSID,
		.ssid_hidden = 0,
		.channel = 3,
		.ssid_len = strlen(AP_SSID),
		.authmode = AUTH_WPA_WPA2_PSK,
		.password = AP_PSK,
		.max_connection = 3,
		.beacon_interval = 100,
	};
	sdk_wifi_softap_set_config(&ap_config);
	ip_addr_t first_client_ip;
	IP4_ADDR(&first_client_ip, 192, 168, 0, 2);
	dhcpserver_start(&first_client_ip, 1);
}

void
wifi_manager_task(void *pvParameters)
{
	mountVol();
	bool connected = false;
	bool clearWificonfig = false;
	sdk_wifi_station_set_auto_connect(0);
	int button = (int) pvParameters;
	gpio_enable(button, GPIO_INPUT);

	if (gpio_read(button) == ACTIVE && false) {
#ifdef WM_DBG
		printf("Factory reset pressed.\n");
#endif
		clearWificonfig = true;
	}
	if (!clearWificonfig && hasWifiConfig()) {
#ifdef WM_DBG
		printf("Trying to connect to AP.\n");
#endif
		connected = init_wifi_client();
		if (!connected) {
			write_file(FILE_WIFI_CONF, "", 1);
			sdk_system_restart();
		}

	}
	if (!connected) {
#ifdef WM_DBG
		printf("Starting http server.\n");
#endif
		init_wifi_ap();
		startConfigManager((void *) &post_recv_callback);
		for (;;) {
			vTaskDelay(10000 / portTICK_PERIOD_MS);
		}
	}
	vTaskDelete(NULL);
}

void
WifiManager(int button, QueueHandle_t * user_main_queue)
{
	xTaskCreate(&wifi_manager_task, "WifiManagerTask", 1024, &button, 2, NULL);
}

/* Json comparison
 */
static int
jsoneq(const char *json, jsmntok_t * tok, const char *s)
{
	if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
	    strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
		return 0;
	}
	return -1;
}

/* Function to parser json string to an structure
 * @param json_data - json string to be parsed
 */
char
parseJSON(char *json_data)
{
#ifdef WM_DBG
	printf("POST data: %s\n", json_data);
#endif
	int i;
	int r;
	jsmn_parser p;
	jsmntok_t t[128];	/* We expect no more than 128 tokens */

	jsmn_init(&p);
	r = jsmn_parse(&p, json_data, strlen(json_data), t,
		       sizeof(t) / sizeof(t[0]));
	if (r < 0) {
#ifdef WM_DBG
		printf("Failed to parse JSON: %d\n", r);
#endif
		return ERROR_JSON_PARSE_FAILED;
	}

	/* Assume the top-level element is an object */
	if (r < 1 || t[0].type != JSMN_OBJECT) {
#ifdef WM_DBG
		printf("Object expected\n");
#endif
		return ERROR_NOT_A_JSON_OBJECT;
	}

	/* Loop over all keys of the root object */
	for (i = 1; i < r; i++) {
		if (jsoneq(json_data, &t[i], keys[0]) == 0) {
			/* We may use strndup() to fetch string value */
			sprintf((char *) config.ssid, "%.*s",
				t[i+1].end-t[i+1].start, json_data + t[i+1].start);
			i++;
		} else if (jsoneq(json_data, &t[i], keys[1]) == 0) {
			/* We may additionally check if the value is either "true"
			 * or "false" */
			sprintf((char *) config.password, "%.*s", t[i+1].end-t[i+1].start,
					json_data + t[i+1].start);
			i++;
		}
	}
	if (!strcmp((char *) config.ssid, "") ||
	     !strcmp((char *) config.password, ""))
	{
#ifdef WM_DBG
		printf("JSON missing field.\n");
#endif
		return ERROR_FIELD_MISSING;
	}
	return ERROR_OK;
}

/* Callback called when POST is received
 * @json_data - json string 
 */
void
post_recv_callback(char *json_data)
{
	if (parseJSON(json_data) != ERROR_OK)
		return;
#ifdef WM_DBG
	printf("data: %s\tdata_len: %d\n", json_data, strlen(json_data));
#endif
	write_file(FILE_WIFI_CONF, json_data, strlen(json_data));
	sdk_system_restart();
}
