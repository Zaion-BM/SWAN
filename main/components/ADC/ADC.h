/*
 * ADC.h
 *
 *  Created on: Mar 19, 2020
 *      Author: mate.berta
 */


#ifndef ADC_H
#define ADC_H

#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern int humidityPercent;
extern TaskHandle_t ADCTaskHandler;
extern TaskHandle_t I2CTaskHandler;
void ADC_Task();

#endif /*ADC_H*/
