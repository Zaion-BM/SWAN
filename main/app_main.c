/* MQTT (over TCP) Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "tcpip_adapter.h"
#include "protocol_examples_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_sntp.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_attr.h"
#include "esp_sleep.h"

#include "protocol_examples_common.h"

#include "mqtt_client.h"

#include "driver/gpio.h"
#include "sdkconfig.h"

#include "esp_pm.h"

//Custom headers
#include "components/ADC/ADC.h"
#include "components/I2C/I2C.h"

//String for ESP debug port
static const char *TAG = "SWAN_CLIENT";

//Defines
#define LED 2
#define valveLED 32
#define BUTTON 23

//Global variables
int humidityPercent = 0;
double temperature = 0;
double pressure = 0;
int valveBAN = 0;
int manualIrrigation = 0;
int valveState = 0;

//Task handlers for custom scheduling
TaskHandle_t ADCTaskHandler;
TaskHandle_t I2CTaskHandler;
TaskHandle_t MQTTTaskHandler;

//Time keeping variables
char strftime_buf[64]; //date
struct tm timeinfo = { 0 };
time_t now = 0; //global time



static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            msg_id = esp_mqtt_client_subscribe(client, "/topic/qos0", 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            if(strcmp(event->data,"1")){
            	printf("Manual irrigation on\r\n");
            	manualIrrigation = 1;
            }
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

 void mqtt_app_start(void *clientptr )
{
	char humidityData[32]=" ";
	char tempData[32]= " ";
	char pressureData[32]= " ";
	char msg[128] =" ";

	esp_mqtt_client_handle_t client = *(esp_mqtt_client_handle_t*) clientptr;

    while(1){
    	while( (pressure==0)) {vTaskDelay(1000/portTICK_PERIOD_MS);}

    	sprintf(humidityData, "%d", humidityPercent);
    	sprintf(tempData, "%f", temperature);
    	sprintf(pressureData, "%f", pressure);

    	time(&now);
    	localtime_r(&now, &timeinfo);
    	strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    	strcpy(msg,strftime_buf);

    	strcat(msg,",");
    	strcat(msg,humidityData);
    	strcat(msg,",");
    	strcat(msg,tempData);
    	strcat(msg,",");
    	strcat(msg,pressureData);
    	strcat(msg,",");
    	if(valveState == 1){strcat(msg,"On");}
    	if(valveState == 0){strcat(msg,"Off");}

    	esp_mqtt_client_publish(client, "/topic/qos1",msg, 0, 1, 0);
    	strcpy(msg," ");

    	vTaskDelay(30000/portTICK_PERIOD_MS);
    }
}

#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_CUSTOM
void sntp_sync_time(struct timeval *tv)
{
   settimeofday(tv, NULL);
   ESP_LOGI(TAG, "Time is synchronized from custom code");
   sntp_set_sync_status(SNTP_SYNC_STATUS_COMPLETED);
}
#endif

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

 static void initialize_sntp(void)
 {
     ESP_LOGI(TAG, "Initializing SNTP");
     sntp_setoperatingmode(SNTP_OPMODE_POLL);
     sntp_setservername(0, "pool.ntp.org");
     sntp_set_time_sync_notification_cb(time_sync_notification_cb);
 #ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
     sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
 #endif
     sntp_init();
 }

void GPIO_Task(void *clientptr){
    gpio_pad_select_gpio(valveLED);
    gpio_set_direction(valveLED, GPIO_MODE_OUTPUT);
    gpio_set_level(valveLED, 0);
    valveState = 0;

    char humidityData[32]=" ";
   	char tempData[32]= " ";
   	char pressureData[32]= " ";
   	char msg[128] =" ";
   	struct tm *tmp;

    esp_mqtt_client_handle_t client = *(esp_mqtt_client_handle_t*) clientptr;

	while(1){
		time(&now);
		localtime_r(&now, &timeinfo);
		strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
		tmp = gmtime(&now);
		printf("%d",tmp->tm_hour);
		//if((tmp->tm_hour<6) || (tmp->tm_hour>23))  {valveBAN =1;}
		//if((tmp->tm_hour>10) && (tmp->tm_hour<18)) {valveBAN =1;}

		if((manualIrrigation==1) && (valveBAN==0) ){

			manualIrrigation = 0;
			gpio_set_level(valveLED, 1);
			valveState=1;

			sprintf(humidityData, "%d", humidityPercent);
			sprintf(tempData, "%f", temperature);
			sprintf(pressureData, "%f", pressure);

	    	strcpy(msg,strftime_buf);
		 	strcat(msg,",");
		 	strcat(msg,humidityData);
		 	strcat(msg,",");
	    	strcat(msg,tempData);
	    	strcat(msg,",");
	    	strcat(msg,pressureData);
			strcat(msg,",On");

			esp_mqtt_client_publish(client, "/topic/qos1",msg, 0, 1, 0);
			strcpy(msg," ");

			vTaskDelay(900000/portTICK_PERIOD_MS);
		}
		else{
			if((humidityPercent<50) && (valveBAN == 0)){
				valveBAN = 1;
				gpio_set_level(valveLED, 1);
				valveState = 1;
				vTaskDelay(300000/portTICK_PERIOD_MS);
			}
			else{
				gpio_set_level(valveLED, 0);
				valveState = 0;
				vTaskDelay(60000/portTICK_PERIOD_MS);
				valveBAN = 0;
			}
		}
	vTaskDelay(1000/portTICK_PERIOD_MS);
	}
}

void app_main()
{
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    initialize_sntp();

    // wait for time to be set
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
           ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
           vTaskDelay(2000 / portTICK_PERIOD_MS);
       }

    time(&now);

    // Set timezone to CET and print local time
    setenv("TZ", "CET-02:00:00CEST-03:00:00,M10.1.0,M3.3.0", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in Budapest is: %s", strftime_buf);

    //Setting up mqtt
    esp_mqtt_client_config_t mqtt_cfg = {.uri = CONFIG_BROKER_URL,};
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
    const esp_mqtt_client_handle_t* clientptr = &client;

    xTaskCreate(ADC_Task,"ADC_read", 3*1024, NULL, 5, NULL);
    xTaskCreate(I2C_Task,"I2C_Task", 3*1024, NULL, 5, NULL);
    xTaskCreate(mqtt_app_start,"MQTT Task",3*1024,(void *) clientptr,5,NULL);
    xTaskCreate(GPIO_Task,"GPIO_Task",3*1024,(void *) clientptr,4,NULL);

    while(1){
    	vTaskDelay(1000/portTICK_PERIOD_MS);
    }


}
