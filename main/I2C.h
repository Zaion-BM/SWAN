/*
 * I2C.h
 *
 *  Created on: Mar 21, 2020
 *      Author: mate.berta
 */

#ifndef MAIN_I2C_H_
#define MAIN_I2C_H_

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "driver/gpio.h"
#include <stdio.h>
#include "bmp280.h"
#include "esp_err.h"

#define SDA_GPIO 21
#define SCL_GPIO 22

void delay_ms(uint32_t period_ms);

int8_t i2c_reg_write(uint8_t i2c_addr, uint8_t reg_addr, uint8_t *reg_data, uint16_t length);

int8_t i2c_reg_read(uint8_t i2c_addr, uint8_t reg_addr, uint8_t *reg_data, uint16_t length);

void print_rslt(const char api_name[], int8_t rslt);

void I2C_Task();

#endif /* MAIN_I2C_H_ */
