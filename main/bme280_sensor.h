#ifndef BME280_SENSOR_H
#define BME280_SENSOR_H

#include "esp_err.h"

typedef struct {
    float temperature; // em °C
    float pressure;    // em hPa
    float humidity;    // em %
} bme280_data_t;

esp_err_t bme280_init(void);
esp_err_t bme280_read_data(bme280_data_t *data);
void bme280_power_down(void);

#endif