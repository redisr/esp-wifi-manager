#include <espressif/esp_common.h>
#include <esp8266.h>
#include <esp/uart.h>
#include <stdio.h>
#include <FreeRTOS.h>
#include <task.h>
#include "queue.h"
#include "WifiManager.h"

#define LED_PIN 2
#define CONFIG_BUTTON 4


void myTask (void *pvParameters)
{
    while(check_wifi_connection() != STATION_GOT_IP);

    for (;;) {
        //do_something usefull
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}

void user_init(void)
{
    uart_set_baud(0, 115200);
    printf("SDK version:%s\n", sdk_system_get_sdk_version());

    QueueHandle_t mainqueue = xQueueCreate(10, sizeof(uint32_t));
    WifiManager(CONFIG_BUTTON, &mainqueue);
}
