#include "telemetry.h"
#include <stdio.h>
#include <time.h>

void telemetry_fill(telemetry_data_t *telemetry, const bme280_data_t *bme, int luminosity)
{
    telemetry->timestamp = (uint64_t)time(NULL);
    telemetry->temperature = bme->temperature;
    telemetry->pressure = bme->pressure;
    telemetry->humidity = bme->humidity;
    telemetry->luminosity = luminosity;
}

int telemetry_format_json(const telemetry_data_t *telemetry, char *buffer, size_t buffer_size)
{
    return snprintf(
        buffer,
        buffer_size,
        "{\"device_id\":\"%s\",\"timestamp\":%llu,\"metrics\":{\"temperature\":%.2f,\"relative_humidity\":%.2f,\"luminosity\":%d,\"pressure\":%.2f}}",
        DEVICE_ID,
        (unsigned long long)telemetry->timestamp,
        telemetry->temperature,
        telemetry->humidity,
        telemetry->luminosity,
        telemetry->pressure
    );
}
