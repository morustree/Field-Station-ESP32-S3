#ifndef STORAGE_H
#define STORAGE_H

#include "esp_err.h"
#include <stddef.h>
#include <stdbool.h>

esp_err_t storage_init(void);
esp_err_t storage_append_backup(const char *json_line);
esp_err_t storage_read_next_backup(char *out_buffer, size_t buffer_size);
esp_err_t storage_delete_next_backup(void);
bool storage_has_pending(void);

#endif // STORAGE_H
