/*
 * I2C.c
 *
 *  Created on: Mar 21, 2020
 *      Author: pep
 */

#include "I2C.h"
/*!
 *
 *
 *  @param[in] period_ms  : the required wait time in milliseconds.
 *  @return void.
 *
 */
void delay_ms(uint32_t period_ms)
{
    /* Implement the delay routine according to the target machine */
	ets_delay_us(period_ms * 1000);
}

void i2c_master_init() {
    i2c_config_t i2c_config = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = SDA_GPIO,
            .scl_io_num = SCL_GPIO,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master.clk_speed = 100000
    };
    i2c_param_config(I2C_NUM_0, &i2c_config);
    printf("Driver install... ");
    printf("%s\n",esp_err_to_name(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0)));
}

/*!
 *  @brief Function for writing the sensor's registers through I2C bus.
 *
 *  @param[in] i2c_addr : sensor I2C address.
 *  @param[in] reg_addr : Register address.
 *  @param[in] reg_data : Pointer to the data buffer whose value is to be written.
 *  @param[in] length   : No of bytes to write.
 *
 *  @return Status of execution
 *  @retval 0 -> Success
 *  @retval >0 -> Failure Info
 *
 */
int8_t i2c_reg_write(uint8_t i2c_addr, uint8_t reg_addr, uint8_t *reg_data, uint16_t length)
{

	/* Implement the I2C write routine according to the target machine. */
	    int8_t iError;
	    esp_err_t esp_err;
	    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
	    i2c_master_start(cmd_handle);
	    i2c_master_write_byte(cmd_handle, (i2c_addr << 1) | I2C_MASTER_WRITE, true);
	    i2c_master_write_byte(cmd_handle, reg_addr, true);
	    i2c_master_write(cmd_handle, reg_data, length,true);
	    i2c_master_stop(cmd_handle);
	    esp_err = i2c_master_cmd_begin(I2C_NUM_0, cmd_handle, 1000 / portTICK_PERIOD_MS);
	    if (esp_err == ESP_OK) {
	        iError = 0;
	    } else {
	        iError = -1;
	    }
	    i2c_cmd_link_delete(cmd_handle);
	    return iError;
}

/*!
 *  @brief Function for reading the sensor's registers through I2C bus.
 *
 *  @param[in] i2c_addr : Sensor I2C address.
 *  @param[in] reg_addr : Register address.
 *  @param[out] reg_data    : Pointer to the data buffer to store the read data.
 *  @param[in] length   : No of bytes to read.
 *
 *  @return Status of execution
 *  @retval 0 -> Success
 *  @retval >0 -> Failure Info
 *
 */
int8_t i2c_reg_read(uint8_t i2c_addr, uint8_t reg_addr, uint8_t *reg_data, uint16_t length)
{
	/* Implement the I2C read routine according to the target machine. */
		int8_t iError;
	    esp_err_t esp_err;
	    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();

	    i2c_master_start(cmd_handle);
	    i2c_master_write_byte(cmd_handle, (i2c_addr << 1) | I2C_MASTER_WRITE, true);
	    i2c_master_write_byte(cmd_handle, reg_addr, true);

	    i2c_master_start(cmd_handle);
	    i2c_master_write_byte(cmd_handle, (i2c_addr << 1) | I2C_MASTER_READ, true);

	    if (length > 1) {
	        i2c_master_read(cmd_handle, reg_data, length - 1, I2C_MASTER_ACK);
	    }
	    i2c_master_read_byte(cmd_handle, reg_data + length - 1, I2C_MASTER_NACK);
	    i2c_master_stop(cmd_handle);

	    esp_err = i2c_master_cmd_begin(I2C_NUM_0, cmd_handle, 1000 / portTICK_PERIOD_MS);

	    if (esp_err == ESP_OK) {
	        iError = 0;
	    } else {
	        iError = -1;
	    }

	    i2c_cmd_link_delete(cmd_handle);


	    return iError;
}


/*!
 *  @brief Prints the execution status of the APIs.
 *
 *  @param[in] api_name : name of the API whose execution status has to be printed.
 *  @param[in] rslt     : error code returned by the API whose execution status has to be printed.
 *
 *  @return void.
 */
void print_rslt(const char api_name[], int8_t rslt)
{
    if (rslt != BMP280_OK)
    {
        printf("%s\t", api_name);
        if (rslt == BMP280_E_NULL_PTR)
        {
            printf("Error [%d] : Null pointer error\r\n", rslt);
        }
        else if (rslt == BMP280_E_COMM_FAIL)
        {
            printf("Error [%d] : Bus communication failed\r\n", rslt);
        }
        else if (rslt == BMP280_E_IMPLAUS_TEMP)
        {
            printf("Error [%d] : Invalid Temperature\r\n", rslt);
        }
        else if (rslt == BMP280_E_DEV_NOT_FOUND)
        {
            printf("Error [%d] : Device not found\r\n", rslt);
        }
        else
        {
            /* For more error codes refer "*_defs.h" */
            printf("Error [%d] : Unknown error code\r\n", rslt);
        }
    }
}


/*!
 *  @brief Example shows basic application to configure and read the temperature.
 */

void I2C_Task(){
	printf("I2C_Task start\n");
	i2c_master_init();
	printf("i2c_master_init finished\n");

	int8_t rslt;
	struct bmp280_dev bmp;
	struct bmp280_config conf;
	struct bmp280_uncomp_data ucomp_data;
	int32_t temp32;
	double temp;
	uint32_t pres32;
	uint32_t pres64;
	double pres;

	/* Map the delay function pointer with the function responsible for implementing the delay */
	bmp.delay_ms = delay_ms;

	/* Assign device I2C address based on the status of SDO pin (GND for PRIMARY(0x76) & VDD for SECONDARY(0x77)) */
	bmp.dev_id = BMP280_I2C_ADDR_PRIM;

	/* Select the interface mode as I2C */
	bmp.intf = BMP280_I2C_INTF;

	/* Map the I2C read & write function pointer with the functions responsible for I2C bus transfer */
	bmp.read = i2c_reg_read;
	bmp.write = i2c_reg_write;

	rslt = bmp280_init(&bmp);
	print_rslt(" bmp280_init status", rslt);

	/* Always read the current settings before writing, especially when
	 * all the configuration is not modified
	 */
	rslt = bmp280_get_config(&conf, &bmp);
	print_rslt(" bmp280_get_config status", rslt);

	/* configuring the temperature oversampling, filter coefficient and output data rate */
	/* Overwrite the desired settings */
	conf.filter = BMP280_FILTER_COEFF_2;

	/* Temperature oversampling set at 4x */
	conf.os_temp = BMP280_OS_4X;

	/* Pressure over sampling none (disabling pressure measurement) */
	conf.os_pres = BMP280_OS_4X;

	/* Setting the output data rate as 1HZ(1000ms) */
    conf.odr = BMP280_ODR_1000_MS;
    rslt = bmp280_set_config(&conf, &bmp);
    print_rslt(" bmp280_set_config status", rslt);

    /* Always set the power mode after setting the configuration */
    rslt = bmp280_set_power_mode(BMP280_NORMAL_MODE, &bmp);
    print_rslt(" bmp280_set_power_mode status", rslt);

    /*Waits for measurment values in the registers are valid*/
    vTaskDelay(1000 / portTICK_PERIOD_MS);



    while (1){

        /* Reading the raw data from sensor */
        rslt = bmp280_get_uncomp_data(&ucomp_data, &bmp);

        /* Getting the 32 bit compensated temperature */
        rslt = bmp280_get_comp_temp_32bit(&temp32, ucomp_data.uncomp_temp, &bmp);

        /* Getting the compensated temperature as floating point value */
        rslt = bmp280_get_comp_temp_double(&temp, ucomp_data.uncomp_temp, &bmp);

        /* Getting the compensated pressure using 32 bit precision */
        rslt = bmp280_get_comp_pres_32bit(&pres32, ucomp_data.uncomp_press, &bmp);

        /* Getting the compensated pressure using 64 bit precision */
        rslt = bmp280_get_comp_pres_64bit(&pres64, ucomp_data.uncomp_press, &bmp);

        /* Getting the compensated pressure as floating point value */
        rslt = bmp280_get_comp_pres_double(&pres, ucomp_data.uncomp_press, &bmp);

        /*Print Pressure Data*/
        printf("UP: %d, P32: %d, P64: %d, P64N: %d, P: %lf\r\n",
           					ucomp_data.uncomp_press,
        	                pres32,
        	                pres64,
        	                pres64 / 256,
        	                pres);

        /*Print Temperature Data*/
        printf("UT: %d, T32: %d, T: %f \r\n", ucomp_data.uncomp_temp, temp32, temp);

        temperature = (double) temp;
        pressure = (double) pres;

        /* Sleep time between measurements = BMP280_ODR_10000_MS */
        vTaskDelay(14999 / portTICK_PERIOD_MS);
    }

}
