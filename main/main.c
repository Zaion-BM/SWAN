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
#include <string.h>

//Custom headers
#include "components/ADC/ADC.h"
#include "components/I2C/I2C.h"

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

TaskHandle_t I2C_Task_handle;

void UART_CMD_Interpreter(){
	while(1){
	char c ='0';
	char cmd[10]="";
	uint8_t cmd_ptr = 0;

	while(c!='\n'){
		c = getchar();
		if((c >= 'a') && (c <= 'z')) {
			cmd[cmd_ptr]=c;
			cmd_ptr++;
		}

		vTaskDelay(100/portTICK_PERIOD_MS);

		if(cmd_ptr==9){
			cmd_ptr=0;
		}
	}
	if(strcmp(cmd,"temp\n")){
		printf("Resume I2C Task\n");
		vTaskResume(I2C_Task_handle);
	}
	printf("%s\n",cmd);
	}
}


void app_main()
{
	xTaskCreate(ADC_Task,"ADC_read", 3*1024, NULL, 5, NULL);
	xTaskCreate(ButtonState,"ButtonState", 3*1024, NULL, 5, NULL);
	xTaskCreate(I2C_Task,"I2C_Task", 3*1024, NULL, 5, &I2C_Task_handle);
	//xTaskCreate(UART_CMD_Interpreter,"UART_CMD_Interpreter", 3*1024, NULL, 5, NULL);
}
