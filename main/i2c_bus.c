#include "i2c_bus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdlib.h>
#include <string.h>

static i2c_master_bus_handle_t s_i2c_bus_handle = NULL;

esp_err_t i2c_bus_init(void)
{
    if (s_i2c_bus_handle != NULL) {
        return ESP_OK;
    }

    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    return i2c_new_master_bus(&i2c_mst_config, &s_i2c_bus_handle);
}

esp_err_t i2c_bus_add_device(uint8_t device_address, i2c_master_dev_handle_t *dev_handle)
{
    if (s_i2c_bus_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = device_address,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };

    return i2c_master_bus_add_device(s_i2c_bus_handle, &dev_cfg, dev_handle);
}

esp_err_t i2c_bus_read_registers(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t *data, size_t len)
{
    if (dev_handle == NULL || data == NULL) return ESP_ERR_INVALID_ARG;

    uint8_t *reg = malloc(1);
    uint8_t *temp_buf = malloc(len);
    if (reg == NULL || temp_buf == NULL) {
        free(reg);
        free(temp_buf);
        return ESP_ERR_NO_MEM;
    }

    reg[0] = reg_addr;
    esp_err_t err = i2c_master_transmit_receive(dev_handle, reg, 1, temp_buf, len, pdMS_TO_TICKS(1000));

    if (err == ESP_OK) {
        memcpy(data, temp_buf, len);
    }

    free(reg);
    free(temp_buf);
    return err;
}

esp_err_t i2c_bus_write_register(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, const uint8_t *data, size_t len)
{
    if (dev_handle == NULL || data == NULL) return ESP_ERR_INVALID_ARG;

    size_t write_len = len + 1;
    uint8_t *write_buffer = malloc(write_len);
    if (write_buffer == NULL) {
        return ESP_ERR_NO_MEM;
    }

    write_buffer[0] = reg_addr;
    memcpy(write_buffer + 1, data, len);
    esp_err_t err = i2c_master_transmit(dev_handle, write_buffer, write_len, pdMS_TO_TICKS(1000));
    free(write_buffer);
    return err;
}
