#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "simcom7020.h"
#include "common.h"

#define SENSOR_PIN  4
#define URI_MQTT         "mqtt://mqtt.innoway.vn"
#define DEVICE_ID        "2fea78f0-f89b-4c54-8f89-ed6ca2650cce"
#define DEVICE_TOKEN     "j8zOkEdc6sQcypjXs2b9Z6Uigo5GLhbW"

struct timeval epoch;
struct timezone utc;

static const char *TAG_SIM = "SIMCOM";
static char topic_pub[100] = {0};
static char topic_sub[100] = {0};

DT DateTime;
uint8_t Datetime[20];
uint32_t freq = 0;
uint64_t current_time = 0;

client client_mqtt = {
    URI_MQTT,
    1883,
    DEVICE_ID,
    DEVICE_ID,
    DEVICE_TOKEN,
    0
};

static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    freq++;
}

void init_gpio_input(gpio_num_t gpio_num) {
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = 1ull << gpio_num;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(gpio_num, gpio_isr_handler, (void*) gpio_num);
}

void publish_task(void * pvParameters) {
    while (1) {
        if (esp_timer_get_time()/1000 - current_time > 2000) {
        	current_time = esp_timer_get_time()/1000;
            ESP_LOGI("MAIN", "CNT: %d", freq);
            freq = 0;
            // Pulse frequency (Hz) = 7.5Q, Q is flow rate in L/min.
            float flowRate = (freq / 7.5);
            // (Pulse frequency x 60 min) / 7.5Q = flowrate in L/hour
            float waterVolume = (flowRate / 60.0) * 1000;
            char data[100] = {0};
            sprintf(data, "{\"t\":%d,\"wv\":%.2f,\"wf\":%.2f}", gettimeofday(&epoch, 0), waterVolume, flowRate);
            mqtt_message_publish(client_mqtt, data, topic_pub, 0, 3);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void app_main(void)
{
	init_gpio_input(SENSOR_PIN);
    init_simcom(ECHO_UART_PORT_NUM_1, ECHO_TEST_TXD_1, ECHO_TEST_RXD_1, ECHO_UART_BAUD_RATE);

    sprintf(topic_pub, "messages/%s/info", DEVICE_ID);
	sprintf(topic_sub, "commands/%s/device/controls", DEVICE_ID);

	Conver_DateTime((char *) __TIME__, 'T');
	Conver_DateTime((char *) __DATE__, 'D');
	sprintf((char *)Datetime, "%d-%d-%d %d:%d:%d", DateTime.year, DateTime.month, DateTime.day,DateTime.hour,DateTime.minute, DateTime.second);
	epoch.tv_sec = string_to_seconds((const char *)Datetime);
	settimeofday(&epoch, &utc);
	printf ("time: %s\r\n", Datetime);
	int retry = 5;
	while (retry--) {
		if (waitModuleReady(50000)) {
			ESP_LOGI(TAG_SIM, "MODULE SIM is REGISTED NETWORK");
			break;
		}
		else restart_simcom();
	}
	if (retry == 0) ESP_LOGI(TAG_SIM, "MODULE SIM is REGISTED NOT NETWORK");

	vTaskDelay(1000/portTICK_PERIOD_MS);
	if (isInit(3)) {
		ESP_LOGI(TAG_SIM, "SIMCOM IS OK");

		if (mqtt_start(client_mqtt, 3, 60, 1, 3)) {
			ESP_LOGI(TAG_SIM, "MQTT IS CONNECTED");
			if (mqtt_subscribe(client_mqtt, topic_sub, 0, 3)) {
				ESP_LOGI(TAG_SIM, "Sent subcribe is successfully");
				mqtt_message_publish(client_mqtt, "hello", topic_pub, 0, 3);
			} else {
				ESP_LOGI(TAG_SIM, "Sent subcribe is failed");
			}
			xTaskCreate(publish_task, "publish_task", 4096, NULL, 5, NULL);
		} else {
			ESP_LOGI(TAG_SIM, "MQTT IS NOT CONNECTED");
		}

	} else {
		ESP_LOGI(TAG_SIM, "SIMCOM IS NOT OK");
	}
}
