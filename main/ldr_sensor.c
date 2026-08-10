#include "ldr_sensor.h"
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ADC_SAMPLES 16
#define LDR_WARMUP_MS 2

esp_err_t ldr_read_raw(int *out_raw)
{
    if (out_raw == NULL) return ESP_ERR_INVALID_ARG;

    // Configura GPIO de Alimentação
    gpio_config_t pwr_conf = {
        .pin_bit_mask = (1ULL << LDR_POWER_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&pwr_conf);
    gpio_hold_dis(LDR_POWER_PIN);

    // Energiza o divisor de tensão do LDR e aguarda estabilizar
    gpio_set_level(LDR_POWER_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(LDR_WARMUP_MS));

    // Inicializa e configura o driver do ADC Oneshot
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
    };
    if (adc_oneshot_new_unit(&init_config, &adc_handle) != ESP_OK) {
         // Se falhar, pelo menos desliga o pino
         gpio_set_level(LDR_POWER_PIN, 0);
         return ESP_FAIL;
    }

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(adc_handle, LDR_ADC_PIN, &config);

    // Leitura com oversampling
    int adc_sum = 0;
    int raw_read = 0;
    for (int i = 0; i < ADC_SAMPLES; i++) {
        adc_oneshot_read(adc_handle, LDR_ADC_PIN, &raw_read);
        adc_sum += raw_read;
    }

    int adc_value = adc_sum / ADC_SAMPLES;
    *out_raw = adc_value;

    // Desaloca a unidade do ADC e desliga a alimentação para zerar o consumo
    adc_oneshot_del_unit(adc_handle);
    gpio_set_level(LDR_POWER_PIN, 0);
    gpio_hold_en(LDR_POWER_PIN);

    return ESP_OK;
}
