#include "storage.h"
#include "esp_err.h"
#include "esp_littlefs.h"
#include "esp_heap_caps.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *BASE_PATH = "/littlefs";
static const char *PARTITION_LABEL = "storage";
static const char *BACKUP_FILE = "/littlefs/backup.log";
static bool s_mounted = false;

esp_err_t storage_init(void)
{
    esp_vfs_littlefs_conf_t conf = {
        .base_path = BASE_PATH,
        .partition_label = PARTITION_LABEL,
        .format_if_mount_failed = true,
        .dont_mount = false,
    };

    esp_err_t err = esp_vfs_littlefs_register(&conf);
    if (err != ESP_OK) {
        return err;
    }

    s_mounted = true;
    return ESP_OK;
}

esp_err_t storage_append_backup(const char *json_line)
{
    if (!s_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    FILE *file = fopen(BACKUP_FILE, "a");
    if (file == NULL) {
        return ESP_FAIL;
    }

    fprintf(file, "%s\n", json_line);
    fflush(file);
    fclose(file);
    return ESP_OK;
}

bool storage_has_pending(void)
{
    if (!s_mounted) {
        return false;
    }

    struct stat st;
    return stat(BACKUP_FILE, &st) == 0 && st.st_size > 0;
}

esp_err_t storage_read_next_backup(char *out_buffer, size_t buffer_size)
{
    if (!s_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    FILE *file = fopen(BACKUP_FILE, "r");
    if (file == NULL) {
        return ESP_ERR_NOT_FOUND;
    }

    if (fgets(out_buffer, buffer_size, file) == NULL) {
        fclose(file);
        return ESP_ERR_NOT_FOUND;
    }

    char *newline = strchr(out_buffer, '\n');
    if (newline) {
        *newline = '\0';
    }

    fclose(file);
    return ESP_OK;
}

esp_err_t storage_delete_next_backup(void)
{
    if (!s_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    FILE *file = fopen(BACKUP_FILE, "r");
    if (file == NULL) {
        return ESP_ERR_NOT_FOUND;
    }

    const char *temp_path = "/littlefs/backup.tmp";
    FILE *temp = fopen(temp_path, "w");
    if (temp == NULL) {
        fclose(file);
        return ESP_FAIL;
    }

    // Aloca buffer na PSRAM para cópia de linhas durante a remoção
    char *line = (char *)heap_caps_malloc(512, MALLOC_CAP_SPIRAM);
    if (line == NULL) {
        fclose(file);
        fclose(temp);
        return ESP_ERR_NO_MEM;
    }

    bool first_line_skipped = false;
    while (fgets(line, 512, file)) {
        if (!first_line_skipped) {
            first_line_skipped = true;
            continue;
        }
        fputs(line, temp);
    }

    heap_caps_free(line);
    fclose(file);
    fclose(temp);

    remove(BACKUP_FILE);
    if (rename(temp_path, BACKUP_FILE) != 0) {
        return ESP_FAIL;
    }

    return ESP_OK;
}
