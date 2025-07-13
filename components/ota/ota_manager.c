#include "ota_manager.h"
#include "esp_log.h"
#include "esp_https_ota.h"
#include "esp_http_client.h"
#include "esp_app_desc.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "OTA_MANAGER";

// OTA manager state
static struct {
    ota_config_t config;
    ota_event_callback_t event_callback;
    esp_https_ota_handle_t ota_handle;
    bool is_updating;
    int progress;
    SemaphoreHandle_t mutex;
    TaskHandle_t update_task_handle;
} ota_manager = {0};

// Forward declarations
static void ota_update_task(void *pvParameter);
static esp_err_t ota_event_handler(esp_https_ota_event_t event, void *data, size_t data_len);

esp_err_t ota_manager_init(const ota_config_t *config, ota_event_callback_t event_callback)
{
    if (config == NULL) {
        ESP_LOGE(TAG, "Invalid configuration");
        return ESP_ERR_INVALID_ARG;
    }

    if (ota_manager.mutex != NULL) {
        ESP_LOGE(TAG, "OTA manager already initialized");
        return ESP_ERR_INVALID_STATE;
    }

    ota_manager.mutex = xSemaphoreCreateMutex();
    if (ota_manager.mutex == NULL) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    memcpy(&ota_manager.config, config, sizeof(ota_config_t));
    ota_manager.event_callback = event_callback;
    ota_manager.is_updating = false;
    ota_manager.progress = 0;
    ota_manager.ota_handle = NULL;
    ota_manager.update_task_handle = NULL;

    // Note: Global CA store is automatically initialized by ESP-IDF
    // when use_global_ca_store is set to true in HTTP client config

    ESP_LOGI(TAG, "OTA manager initialized");
    return ESP_OK;
}

esp_err_t ota_manager_start_update(void)
{
    if (xSemaphoreTake(ota_manager.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to acquire mutex");
        return ESP_ERR_TIMEOUT;
    }

    if (ota_manager.is_updating) {
        ESP_LOGE(TAG, "OTA update already in progress");
        xSemaphoreGive(ota_manager.mutex);
        return ESP_ERR_INVALID_STATE;
    }

    ota_manager.is_updating = true;
    ota_manager.progress = 0;
    xSemaphoreGive(ota_manager.mutex);

    // Create update task
    BaseType_t ret = xTaskCreate(ota_update_task, "ota_update", 8192, NULL, 5, &ota_manager.update_task_handle);
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create OTA update task");
        ota_manager.is_updating = false;
        return ESP_ERR_NO_MEM;
    }

    ESP_LOGI(TAG, "OTA update started");
    return ESP_OK;
}

bool ota_manager_is_updating(void)
{
    bool is_updating = false;
    if (xSemaphoreTake(ota_manager.mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        is_updating = ota_manager.is_updating;
        xSemaphoreGive(ota_manager.mutex);
    }
    return is_updating;
}

int ota_manager_get_progress(void)
{
    int progress = 0;
    if (xSemaphoreTake(ota_manager.mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        progress = ota_manager.progress;
        xSemaphoreGive(ota_manager.mutex);
    }
    return progress;
}

esp_err_t ota_manager_abort_update(void)
{
    if (xSemaphoreTake(ota_manager.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    if (!ota_manager.is_updating) {
        xSemaphoreGive(ota_manager.mutex);
        return ESP_ERR_INVALID_STATE;
    }

    // Abort OTA if handle exists
    if (ota_manager.ota_handle != NULL) {
        esp_https_ota_abort(ota_manager.ota_handle);
    }

    ota_manager.is_updating = false;
    xSemaphoreGive(ota_manager.mutex);

    ESP_LOGI(TAG, "OTA update aborted");
    return ESP_OK;
}

esp_err_t ota_manager_deinit(void)
{
    if (xSemaphoreTake(ota_manager.mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    if (ota_manager.is_updating) {
        esp_https_ota_abort(ota_manager.ota_handle);
        ota_manager.is_updating = false;
    }

    if (ota_manager.mutex != NULL) {
        vSemaphoreDelete(ota_manager.mutex);
        ota_manager.mutex = NULL;
    }

    // Note: Global CA store is automatically cleaned up by ESP-IDF

    ESP_LOGI(TAG, "OTA manager deinitialized");
    return ESP_OK;
}

static void ota_update_task(void *pvParameter)
{
    esp_err_t err = ESP_OK;
    ota_event_data_t event_data = {0};

    // Notify start
    event_data.event = OTA_EVENT_STARTED;
    if (ota_manager.event_callback) {
        ota_manager.event_callback(&event_data);
    }

    // Configure HTTP client
    esp_http_client_config_t http_config = {
        .url = ota_manager.config.url,
        .cert_pem = ota_manager.config.cert_pem,
        .client_cert_pem = ota_manager.config.client_cert_pem,
        .client_key_pem = ota_manager.config.client_key_pem,
        .username = ota_manager.config.username,
        .password = ota_manager.config.password,
        .timeout_ms = ota_manager.config.timeout_ms,
        .skip_cert_common_name_check = ota_manager.config.skip_cert_common_name_check,
        .use_global_ca_store = false,  // Use embedded certificate instead
        .crt_bundle_attach = NULL,     // Certificate bundle not available in this build
    };

    // Configure HTTPS OTA
    esp_https_ota_config_t ota_config = {
        .http_config = &http_config,
        .http_client_init_cb = NULL,
        .bulk_flash_erase = true,
        .partial_http_download = true,
    };

    // Start HTTPS OTA
    err = esp_https_ota_begin(&ota_config, &ota_manager.ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "ESP HTTPS OTA Begin failed: %s", esp_err_to_name(err));
        event_data.event = OTA_EVENT_FAILED;
        event_data.error_message = "Failed to start OTA";
        if (ota_manager.event_callback) {
            ota_manager.event_callback(&event_data);
        }
        goto cleanup;
    }

    // Get app description
    esp_app_desc_t app_desc;
    err = esp_https_ota_get_img_desc(ota_manager.ota_handle, &app_desc);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_https_ota_read_img_desc failed: %s", esp_err_to_name(err));
        event_data.event = OTA_EVENT_FAILED;
        event_data.error_message = "Failed to read image description";
        if (ota_manager.event_callback) {
            ota_manager.event_callback(&event_data);
        }
        goto cleanup;
    }

    ESP_LOGI(TAG, "New firmware version: %s", app_desc.version);

    // Download and flash firmware
    while (1) {
        err = esp_https_ota_perform(ota_manager.ota_handle);
        if (err != ESP_ERR_HTTPS_OTA_IN_PROGRESS) {
            break;
        }

        // Update progress
        int total_size = esp_https_ota_get_image_size(ota_manager.ota_handle);
        int downloaded_size = esp_https_ota_get_image_len_read(ota_manager.ota_handle);
        
        if (total_size > 0) {
            int progress = (downloaded_size * 100) / total_size;
            if (xSemaphoreTake(ota_manager.mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                ota_manager.progress = progress;
                xSemaphoreGive(ota_manager.mutex);
            }

            event_data.event = OTA_EVENT_PROGRESS;
            event_data.progress_percent = progress;
            if (ota_manager.event_callback) {
                ota_manager.event_callback(&event_data);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }

    if (esp_https_ota_is_complete_data_received(ota_manager.ota_handle) != true) {
        ESP_LOGE(TAG, "Complete data was not received.");
        event_data.event = OTA_EVENT_FAILED;
        event_data.error_message = "Incomplete data received";
        if (ota_manager.event_callback) {
            ota_manager.event_callback(&event_data);
        }
        goto cleanup;
    }

    // Finish OTA
    err = esp_https_ota_finish(ota_manager.ota_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
            event_data.error_message = "Image validation failed";
        } else {
            ESP_LOGE(TAG, "ESP HTTPS OTA finish failed: %s", esp_err_to_name(err));
            event_data.error_message = "Failed to finish OTA";
        }
        event_data.event = OTA_EVENT_FAILED;
        if (ota_manager.event_callback) {
            ota_manager.event_callback(&event_data);
        }
        goto cleanup;
    }

    ESP_LOGI(TAG, "ESP HTTPS OTA upgrade successful. New firmware version: %s", app_desc.version);
    
    // Notify completion
    event_data.event = OTA_EVENT_COMPLETED;
    event_data.progress_percent = 100;
    if (ota_manager.event_callback) {
        ota_manager.event_callback(&event_data);
    }

    // Restart after a short delay
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();

cleanup:
    if (xSemaphoreTake(ota_manager.mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        ota_manager.is_updating = false;
        ota_manager.ota_handle = NULL;
        xSemaphoreGive(ota_manager.mutex);
    }
    
    if (ota_manager.update_task_handle != NULL) {
        vTaskDelete(ota_manager.update_task_handle);
        ota_manager.update_task_handle = NULL;
    }
} 