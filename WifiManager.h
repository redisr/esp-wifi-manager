#define WIFIMANAGER_H

#define AP_SSID "my_ap"
#define AP_PSK "secure_passwd"
#define FILE_WIFI_CONF "/wificonfig"
#define ACTIVE  		1

enum {
	ERROR_FIELD_MISSING,
	ERROR_NOT_A_JSON_OBJECT,
	ERROR_JSON_PARSE_FAILED,
	ERROR_OK
};

/*
 * Load wifi configs or creates an http server to receive the configs via POST
 * at http://192.168.0.1/
 */
void 
WifiManager(int button, QueueHandle_t *user_main_queue);

/*
 * Check wifi connection status
 * @return enum wifi status
 */
uint8_t
check_wifi_connection();