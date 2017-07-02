#include <espressif/esp_common.h>
#include <esp8266.h>
#include <esp/uart.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include <stdlib.h>
#include <httpd/httpd.h>
#include <string.h>
#include "web_config.h"

/*
 * HTTP server task handlers
 */

void 
task_http_conf_server(void *pvParameters)
{
    httpd_init();
    for (;;) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
    }
}

/*
 * Server functions
 */

/* POST request handers */
err_t 
httpd_post_begin(void *connection, const char *uri, const char *http_request,
         u16_t http_request_len, int content_len, char *response_uri,
         u16_t response_uri_len, u8_t * post_auto_wnd)
{
    strcpy(response_uri, "/index.html");
    response_uri_len = strlen(response_uri);
    return ERR_OK;
}

void 
post_recv_task(void *pvParameters)
{
    char *data = (char *) pvParameters;
    config_callback(data);
    vTaskDelete(NULL);
}

err_t 
httpd_post_receive_data(void *connection, struct pbuf *p)
{
    int p_len = p->len;
    char *data = (char *) malloc(p_len * sizeof(char));
    strcpy(data, (char *) p->payload);

    xTaskCreate(post_recv_task, "PostRecvTask", 1024, data, 2, NULL);
    pbuf_free(p);
    return ERR_OK;
}

void
httpd_post_finished(void *connection, char *response_uri, u16_t response_uri_len)
{
    strcpy(response_uri, "/index.html");
}

/*
 * Function to start the the http server
 */
void 
startConfigManager(void (*_func)(char *))
{
    xTaskCreate(&task_http_conf_server, "HTTP Daemon", 1024, NULL, 1, NULL);
    config_callback = _func;
}
