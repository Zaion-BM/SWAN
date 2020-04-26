#include "ADC.h"


void ADC_Task(){

	adc1_config_width(ADC_WIDTH_BIT_12);
	adc1_config_channel_atten(ADC_CHANNEL_0, ADC_ATTEN_DB_11);

	//Characterize ADC at particular attenuation
	esp_adc_cal_characteristics_t *adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, adc_chars);

    //Check type of calibration value used to characterize ADC
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        printf("eFuse Vref\n");
    }
    else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        printf("Two Point\n");
    }
    else {
        printf("Default\n");
    }
    while(1){
    printf("\n");
	uint32_t reading =  adc1_get_raw(ADC1_CHANNEL_0);
	uint32_t voltage = esp_adc_cal_raw_to_voltage(reading, adc_chars);
	printf("ADC1_CH0 value:%d, voltage:%d mV\n",reading,voltage);
	vTaskDelay(10000/portTICK_PERIOD_MS);
    }
}
