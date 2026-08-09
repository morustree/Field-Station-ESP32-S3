#ifndef LDR_SENSOR_H
#define LDR_SENSOR_H

#include "esp_err.h"
#include "hal/adc_types.h"

#define LDR_ADC_PIN      ADC_CHANNEL_3  // Corresponde ao GPIO 4 no ESP32-S3
#define LDR_POWER_PIN    6              // Alimentação chaveada do LDR (GPIO 6)

// Retorna a iluminância bruta (ADC raw value 0-4095)
esp_err_t ldr_read_raw(int *out_raw);

#endif
