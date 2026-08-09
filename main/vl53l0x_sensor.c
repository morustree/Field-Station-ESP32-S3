#include "vl53l0x_sensor.h"
#include "i2c_bus.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static i2c_master_dev_handle_t s_vl53l0x_handle = NULL;

static esp_err_t vl53l0x_xshut_set_level(bool high)
{
    gpio_hold_dis(VL53L0X_XSHUT_PIN);
    gpio_set_level(VL53L0X_XSHUT_PIN, high ? 1 : 0);
    if (!high) {
        gpio_hold_en(VL53L0X_XSHUT_PIN);
    }
    return ESP_OK;
}

esp_err_t vl53l0x_init(void)
{
    gpio_config_t xshut_conf = {
        .pin_bit_mask = (1ULL << VL53L0X_XSHUT_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    if (gpio_config(&xshut_conf) != ESP_OK) return ESP_FAIL;
    vl53l0x_xshut_set_level(false);
    vTaskDelay(pdMS_TO_TICKS(10));
    vl53l0x_xshut_set_level(true);
    vTaskDelay(pdMS_TO_TICKS(20));

    esp_err_t err = i2c_bus_add_device(VL53L0X_I2C_ADDR, &s_vl53l0x_handle);
    if (err != ESP_OK) {
        s_vl53l0x_handle = NULL;
        vl53l0x_xshut_set_level(false);
        return err;
    }

    uint8_t reg = 0x0C;
    uint8_t model_id = 0;
    err = i2c_master_transmit_receive(s_vl53l0x_handle, &reg, 1, &model_id, 1, pdMS_TO_TICKS(100));
    if (err != ESP_OK || model_id != 0xEE) {
        i2c_master_bus_rm_device(s_vl53l0x_handle);
        s_vl53l0x_handle = NULL;
        vl53l0x_xshut_set_level(false);
        return ESP_FAIL;
    }

    uint8_t init_seq[][2] = {
        {0x80, 0x01},
        {0xFF, 0x01},
        {0x00, 0x00},
        {0x91, 0x3C},
        {0x00, 0x01},
        {0xFF, 0x00},
        {0x80, 0x00},
        {0x88, 0x00}, // Refined: Tuning register
        {0x80, 0x01},
        {0xFF, 0x01},
        {0x00, 0x00},
        {0x91, 0x3C},
        {0x00, 0x01},
        {0xFF, 0x00},
        {0x80, 0x00},
        {0x0B, 0x01}, // Enable sensor
        {0x00, 0x01},
        {0x01, 0xFF}
    };

    for (int i = 0; i < sizeof(init_seq)/sizeof(init_seq[0]); i++) {
        err = i2c_master_transmit(s_vl53l0x_handle, init_seq[i], 2, pdMS_TO_TICKS(100));
        if (err != ESP_OK) {
            i2c_master_bus_rm_device(s_vl53l0x_handle);
            s_vl53l0x_handle = NULL;
            vl53l0x_xshut_set_level(false);
            return err;
        }
    }

    // Refined: Performance Tuning from Datasheet
    uint8_t vhv_config = 0;
    i2c_bus_read_registers(s_vl53l0x_handle, 0x01, &vhv_config, 1);
    vhv_config |= 0x40;
    i2c_bus_write_register(s_vl53l0x_handle, 0x01, &vhv_config, 1);

    vl53l0x_xshut_set_level(false);
    return ESP_OK;
}

esp_err_t vl53l0x_read_distance_power_efficient(uint16_t *distance_mm)
{
    if (s_vl53l0x_handle == NULL || distance_mm == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    vl53l0x_xshut_set_level(true);
    vTaskDelay(pdMS_TO_TICKS(50));

    // Refined: Start single ranging measurement
    uint8_t cmd_start[2] = {0x00, 0x01}; // SYSRANGE_START bit 0 = 1
    esp_err_t err = i2c_master_transmit(s_vl53l0x_handle, cmd_start, 2, pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        vl53l0x_xshut_set_level(false);
        return err;
    }

    // Poll for completion (bit 0 becomes 0 when measurement is done or check interrupt status)
    uint8_t status = 0;
    int retry = 0;
    do {
        vTaskDelay(pdMS_TO_TICKS(10));
        uint8_t reg_status = 0x13; // RESULT_INTERRUPT_STATUS
        i2c_master_transmit_receive(s_vl53l0x_handle, &reg_status, 1, &status, 1, pdMS_TO_TICKS(50));
    } while ((status & 0x07) == 0 && retry++ < 20);

    uint8_t reg = 0x1E; // RESULT_RANGE_STATUS + 10 (ambient count, etc)
    uint8_t buffer[12] = {0};
    err = i2c_master_transmit_receive(s_vl53l0x_handle, &reg, 1, buffer, 12, pdMS_TO_TICKS(100));
    if (err == ESP_OK) {
        // Range is at offset 10,11 in the result registers starting at 0x14
        // But if reading from 0x1E, it's index 0 and 1
        *distance_mm = (buffer[0] << 8) | buffer[1];

        // Clear interrupt
        uint8_t clear_int[2] = {0x0B, 0x01};
        i2c_master_transmit(s_vl53l0x_handle, clear_int, 2, pdMS_TO_TICKS(50));
    }

    vl53l0x_xshut_set_level(false);
    return err;
}

void vl53l0x_power_down(void)
{
    vl53l0x_xshut_set_level(false);
    if (s_vl53l0x_handle) {
        i2c_master_bus_rm_device(s_vl53l0x_handle);
        s_vl53l0x_handle = NULL;
    }
}
