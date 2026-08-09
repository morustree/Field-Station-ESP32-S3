#include <stdbool.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_sleep.h"
#include "esp_heap_caps.h"
#include "driver/gpio.h"
#include "i2c_bus.h"
#include "bme280_sensor.h"
#include "vl53l0x_sensor.h"
#include "ldr_sensor.h"
#include "network.h"
#include "storage.h"
#include "telemetry.h"

#define DEEP_SLEEP_TIME_US (5 * 60 * 1000000ULL) // 5 minutos
#define JSON_BUF_SIZE      512

static void drain_pending_backups(void)
{
    if (!network_is_connected() || !storage_has_pending()) {
        return;
    }

    char *pending_json = (char *)heap_caps_malloc(JSON_BUF_SIZE, MALLOC_CAP_SPIRAM);
    if (pending_json == NULL) return;

    while (network_is_connected() && storage_has_pending()) {
        if (storage_read_next_backup(pending_json, JSON_BUF_SIZE) != ESP_OK) {
            break;
        }
        if (network_publish(MQTT_TOPIC, pending_json, 1) == ESP_OK) {
            storage_delete_next_backup();
        } else {
            break;
        }
    }
    heap_caps_free(pending_json);
}

void app_main(void)
{
    // 1. Inicializar Hardware
    i2c_bus_init();
    storage_init();

    // 2. Acionar Sensores
    bme280_init();
    vl53l0x_init();
    vTaskDelay(pdMS_TO_TICKS(250)); // Aumentado para dar tempo aos sensores I2C

    // 3. Medições
    bme280_data_t bme_data = {0};
    uint16_t dist = 0;
    int ldr_raw = 0;

    bme280_read_data(&bme_data);
    vl53l0x_read_distance_power_efficient(&dist);
    ldr_read_raw(&ldr_raw);

    // 4. Conectividade com Sincronização Estrita
    char *json_buf = (char *)heap_caps_malloc(JSON_BUF_SIZE, MALLOC_CAP_SPIRAM);
    if (json_buf == NULL) return;

    telemetry_data_t td = {0};

    if (network_init(WIFI_STA_SSID, WIFI_STA_PASSWORD, MQTT_BROKER_URI) == ESP_OK) {
        // Trava de Tempo Estrita
        if (network_sync_time() == ESP_OK) {
            telemetry_fill(&td, &bme_data, ldr_raw, dist);
            telemetry_format_json(&td, json_buf, JSON_BUF_SIZE);

            if (network_publish(MQTT_TOPIC, json_buf, 1) == ESP_OK) {
                drain_pending_backups();
            } else {
                storage_append_backup(json_buf);
            }
        } else {
            // Se o tempo falhou, não enviamos nada nem salvamos backup "sem data"
        }
        network_disconnect();
    } else {
        // Caso de rede offline total, verificamos se temos tempo no RTC
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);

        if (timeinfo.tm_year > (2020 - 1900)) {
            telemetry_fill(&td, &bme_data, ldr_raw, dist);
            telemetry_format_json(&td, json_buf, JSON_BUF_SIZE);
            storage_append_backup(json_buf);
        } else {
            // Offline e sem tempo. Nada a fazer.
        }
    }

    heap_caps_free(json_buf);

    // 5. Deep Sleep
    bme280_power_down();
    vl53l0x_power_down();

    gpio_hold_en(LDR_POWER_PIN);
    gpio_hold_en(VL53L0X_XSHUT_PIN);

    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_TIME_US);
    esp_deep_sleep_start();
}
