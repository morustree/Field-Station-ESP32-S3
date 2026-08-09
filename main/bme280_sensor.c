#include "bme280_sensor.h"
#include "i2c_bus.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BME280_ADDR 0x76

static i2c_master_dev_handle_t s_bme280_handle = NULL;

static uint16_t dig_T1;
static int16_t  dig_T2, dig_T3;
static uint16_t dig_P1;
static int16_t  dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9;
static uint8_t  dig_H1, dig_H3;
static int16_t  dig_H2;
static int8_t   dig_H4, dig_H5, dig_H6;

static esp_err_t bme280_read_calibration(void)
{
    uint8_t calib[26] = {0};
    esp_err_t err = i2c_bus_read_registers(s_bme280_handle, 0x88, calib, 26);
    if (err != ESP_OK) return err;

    dig_T1 = (calib[1] << 8) | calib[0];
    dig_T2 = (int16_t)((calib[3] << 8) | calib[2]);
    dig_T3 = (int16_t)((calib[5] << 8) | calib[4]);
    dig_P1 = (calib[7] << 8) | calib[6];
    dig_P2 = (int16_t)((calib[9] << 8) | calib[8]);
    dig_P3 = (int16_t)((calib[11] << 8) | calib[10]);
    dig_P4 = (int16_t)((calib[13] << 8) | calib[12]);
    dig_P5 = (int16_t)((calib[15] << 8) | calib[14]);
    dig_P6 = (int16_t)((calib[17] << 8) | calib[16]);
    dig_P7 = (int16_t)((calib[19] << 8) | calib[18]);
    dig_P8 = (int16_t)((calib[21] << 8) | calib[20]);
    dig_P9 = (int16_t)((calib[23] << 8) | calib[22]);
    dig_H1 = calib[25];

    uint8_t h_calib[7] = {0};
    err = i2c_bus_read_registers(s_bme280_handle, 0xE1, h_calib, 7);
    if (err != ESP_OK) return err;

    dig_H2 = (int16_t)((h_calib[1] << 8) | h_calib[0]);
    dig_H3 = h_calib[2];
    dig_H4 = (int8_t)((h_calib[3] << 4) | (h_calib[4] & 0x0F));
    dig_H5 = (int8_t)((h_calib[5] << 4) | (h_calib[4] >> 4));
    dig_H6 = (int8_t)h_calib[6];

    return ESP_OK;
}

esp_err_t bme280_init(void)
{
    if (s_bme280_handle == NULL) {
        esp_err_t err = i2c_bus_add_device(BME280_ADDR, &s_bme280_handle);
        if (err != ESP_OK) return err;
    }

    // Reset via software
    uint8_t reset_cmd[2] = {0xE0, 0xB6};
    i2c_master_transmit(s_bme280_handle, reset_cmd, 2, pdMS_TO_TICKS(50));
    vTaskDelay(pdMS_TO_TICKS(10));

    return bme280_read_calibration();
}

void bme280_power_down(void)
{
    if (s_bme280_handle == NULL) return;
    // Coloca em Sleep Mode (bits 1:0 do reg 0xF4 em 00)
    uint8_t sleep_cmd[2] = {0xF4, 0x00};
    i2c_master_transmit(s_bme280_handle, sleep_cmd, 2, pdMS_TO_TICKS(50));
}

esp_err_t bme280_read_data(bme280_data_t *data)
{
    if (s_bme280_handle == NULL || data == NULL) return ESP_ERR_INVALID_STATE;

    // Configuração de Oversampling e Forced Mode (leitura única)
    uint8_t ctrl_hum = 0x01; // x1
    uint8_t ctrl_meas = 0x25; // Press x1, Temp x1, Forced Mode

    i2c_bus_write_register(s_bme280_handle, 0xF2, &ctrl_hum, 1);
    i2c_bus_write_register(s_bme280_handle, 0xF4, &ctrl_meas, 1);

    vTaskDelay(pdMS_TO_TICKS(15)); // Aguarda medição

    uint8_t raw[8] = {0};
    esp_err_t err = i2c_bus_read_registers(s_bme280_handle, 0xF7, raw, 8);
    if (err != ESP_OK) return err;

    // Fórmulas de compensação omitidas por brevidade, mas mantidas a lógica de cálculo original
    int32_t adc_p = (int32_t)(((uint32_t)raw[0] << 12) | ((uint32_t)raw[1] << 4) | ((uint32_t)raw[2] >> 4));
    int32_t adc_t = (int32_t)(((uint32_t)raw[3] << 12) | ((uint32_t)raw[4] << 4) | ((uint32_t)raw[5] >> 4));
    int32_t adc_h = (int32_t)(((uint32_t)raw[6] << 8) | (uint32_t)raw[7]);

    // [Cálculos de var1, var2, t_fine e p...]
    // (Mantendo os cálculos do arquivo original aqui)
    int32_t var1, var2, t_fine;
    var1 = (((adc_t >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2) >> 11;
    var2 = (((((adc_t >> 4) - ((int32_t)dig_T1)) * ((adc_t >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2;
    data->temperature = (float)((t_fine * 5 + 128) >> 8) / 100.0f;

    var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)dig_P6);
    var2 = var2 + ((var1 * ((int32_t)dig_P5)) << 1);
    var2 = (var2 >> 2) + (((int32_t)dig_P4) << 16);
    var1 = (((dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((int32_t)dig_P2) * var1) >> 1)) >> 18;
    var1 = ((((32768 + var1)) * ((int32_t)dig_P1)) >> 15);
    if (var1 != 0) {
        uint32_t p = (((uint32_t)(((int32_t)1048576) - adc_p) - (var2 >> 12))) * 3125;
        if (p < 0x80000000) p = (p << 1) / ((uint32_t)var1);
        else p = (p / (uint32_t)var1) * 2;
        var1 = (((int32_t)dig_P9) * ((int32_t)(((p >> 3) * (p >> 3)) >> 13))) >> 12;
        var2 = (((int32_t)(p >> 2)) * ((int32_t)dig_P8)) >> 13;
        data->pressure = (float)((int32_t)p + ((var1 + var2 + dig_P7) >> 4)) / 100.0f;
    }

    int32_t v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_h << 14) - (((int32_t)dig_H4) << 20) - (((int32_t)dig_H5) * v_x1_u32r)) +
                ((int32_t)16384)) >> 15) * (((((((v_x1_u32r * ((int32_t)dig_H6)) >> 10) *
                (((v_x1_u32r * ((int32_t)dig_H3)) >> 11) + ((int32_t)32768))) >> 10) +
                ((int32_t)2097152)) * ((int32_t)dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)dig_H1)) >> 4));
    if (v_x1_u32r < 0) v_x1_u32r = 0;
    if (v_x1_u32r > 419430400) v_x1_u32r = 419430400;
    data->humidity = (float)(v_x1_u32r >> 12) / 1024.0f;

    return ESP_OK;
}
