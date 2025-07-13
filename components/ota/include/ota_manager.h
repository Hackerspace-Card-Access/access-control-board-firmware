#pragma once

#include "esp_err.h"
#include "esp_https_ota.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OTA configuration structure
 */
typedef struct {
    const char *url;                    // HTTPS URL for firmware update
    const char *cert_pem;               // Server certificate (PEM format)
    const char *client_cert_pem;        // Client certificate (PEM format) - optional
    const char *client_key_pem;         // Client private key (PEM format) - optional
    const char *username;               // HTTP Basic Auth username - optional
    const char *password;               // HTTP Basic Auth password - optional
    int timeout_ms;                     // HTTP timeout in milliseconds
    bool skip_cert_common_name_check;   // Skip certificate common name check
} ota_config_t;

/**
 * @brief OTA event types
 */
typedef enum {
    OTA_EVENT_STARTED,
    OTA_EVENT_PROGRESS,
    OTA_EVENT_COMPLETED,
    OTA_EVENT_FAILED
} ota_event_t;

/**
 * @brief OTA event data structure
 */
typedef struct {
    ota_event_t event;
    int progress_percent;               // Progress percentage (0-100)
    const char *error_message;          // Error message if event is OTA_EVENT_FAILED
} ota_event_data_t;

/**
 * @brief OTA event callback function type
 */
typedef void (*ota_event_callback_t)(ota_event_data_t *event_data);

/**
 * @brief Initialize OTA manager
 * 
 * @param config OTA configuration
 * @param event_callback Optional callback function for OTA events
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t ota_manager_init(const ota_config_t *config, ota_event_callback_t event_callback);

/**
 * @brief Start OTA update process
 * 
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t ota_manager_start_update(void);

/**
 * @brief Check if OTA update is in progress
 * 
 * @return true if update is in progress, false otherwise
 */
bool ota_manager_is_updating(void);

/**
 * @brief Get current OTA progress percentage
 * 
 * @return int Progress percentage (0-100)
 */
int ota_manager_get_progress(void);

/**
 * @brief Abort current OTA update
 * 
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t ota_manager_abort_update(void);

/**
 * @brief Deinitialize OTA manager
 * 
 * @return esp_err_t ESP_OK on success, error code otherwise
 */
esp_err_t ota_manager_deinit(void);

#ifdef __cplusplus
}
#endif 