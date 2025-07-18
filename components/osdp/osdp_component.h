/*
 * OSDP Component Header
 * 
 * This file provides the interface for the OSDP component functionality.
 */

#ifndef OSDP_COMPONENT_H
#define OSDP_COMPONENT_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the OSDP component
 * 
 * This function initializes the OSDP (Open Supervised Device Protocol) component.
 * It sets up the necessary data structures, configurations, and prepares the
 * component for operation.
 * 
 * @return esp_err_t ESP_OK on success, error code on failure
 */
esp_err_t osdp_init(void);

#ifdef __cplusplus
}
#endif

#endif /* OSDP_COMPONENT_H */ 