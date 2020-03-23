/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

//Custom headers
#include "ADC.h"
#include "I2C.h"

//Defines
#define LED 2
#define BUTTON 23

void ButtonState(){

	/*Select pins to be configured as GPIOs*/
		gpio_pad_select_gpio(LED);
		gpio_pad_select_gpio(BUTTON);
	/* Set GPIOs as input/output*/
		gpio_set_direction(LED, GPIO_MODE_OUTPUT);
		gpio_set_direction(BUTTON, GPIO_MODE_INPUT);

    while(1) {
    	//printf("\n");
    	if(gpio_get_level(BUTTON)){
    		/* LED on */
			//printf("Turning on the LED\n");
	        gpio_set_level(LED, 1);
	        vTaskDelay(1000 / portTICK_PERIOD_MS);
    	}
    	else{
	        /* LED off */
	        //printf("Turning off the LED\n");
	        gpio_set_level(LED, 0);
	        vTaskDelay(1000 / portTICK_PERIOD_MS);
    	}
	  }

}


void app_main()
{
	xTaskCreate(ADC_Task,"ADC_read", 3*1024, NULL, 5, NULL);
	xTaskCreate(ButtonState,"ButtonState", 3*1024, NULL, 5, NULL);
	xTaskCreate(I2C_Task,"I2C_Task", 3*1024, NULL, 5, NULL);

}
