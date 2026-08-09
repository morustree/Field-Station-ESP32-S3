#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "driver/i2c_master.h"
#include "esp_err.h"

#define I2C_MASTER_NUM         I2C_NUM_0
#define I2C_MASTER_SDA_IO      8
#define I2C_MASTER_SCL_IO      9
#define I2C_MASTER_FREQ_HZ     100000

esp_err_t i2c_bus_init(void);
esp_err_t i2c_bus_add_device(uint8_t device_address, i2c_master_dev_handle_t *dev_handle);
esp_err_t i2c_bus_read_registers(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t *data, size_t len);
esp_err_t i2c_bus_write_register(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, const uint8_t *data, size_t len);

#endif