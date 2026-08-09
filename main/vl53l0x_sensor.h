#ifndef VL53L0X_SENSOR_H
#define VL53L0X_SENSOR_H

#include "esp_err.h"
#include <stdint.h>

#define VL53L0X_I2C_ADDR 0x29
#define VL53L0X_XSHUT_PIN GPIO_NUM_12

esp_err_t vl53l0x_init(void);
esp_err_t vl53l0x_read_distance_power_efficient(uint16_t *distance_mm);
void vl53l0x_power_down(void);

#endif