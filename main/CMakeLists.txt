idf_component_register(
    SRCS "main.c" "i2c_bus.c" "ldr_sensor.c" "bme280_sensor.c" "vl53l0x_sensor.c" "network.c" "storage.c" "telemetry.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_gpio esp_driver_i2c esp_adc esp_wifi esp_netif esp_event mqtt nvs_flash esp_timer esp_hw_support
    PRIV_REQUIRES littlefs
)
