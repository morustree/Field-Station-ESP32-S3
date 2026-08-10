#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdint.h>
#include <stddef.h>
#include "bme280_sensor.h"
#include "secrets.h"

typedef struct {
    uint64_t timestamp;
    float temperature;
    float pressure;
    float humidity;
    int luminosity;
} telemetry_data_t;

void telemetry_fill(telemetry_data_t *telemetry, const bme280_data_t *bme, int luminosity);
int telemetry_format_json(const telemetry_data_t *telemetry, char *buffer, size_t buffer_size);

#endif // TELEMETRY_H
