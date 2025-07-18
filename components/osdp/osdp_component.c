/*
 * OSDP Component Implementation
 * 
 * This file contains the implementation of the OSDP component functionality.
 */

#include "osdp_component.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "string.h"
#include "stdlib.h"

static const char *TAG = "ACCESS_CTL_OSDP";

// Global variables for OSDP component
static uart_port_t osdp_uart_port = UART_NUM_2;
static bool osdp_initialized = false;
static QueueHandle_t osdp_rx_queue = NULL;
static TaskHandle_t osdp_task_handle = NULL;

// RS485 control functions
static void rs485_set_tx_mode(void)
{
    // Set DE (Driver Enable) high and RE (Receiver Enable) low for transmit mode
    gpio_set_level(CONFIG_OSDP_RS485_DE_GPIO, 1);
    gpio_set_level(CONFIG_OSDP_RS485_RE_GPIO, 0);
}

static void rs485_set_rx_mode(void)
{
    // Set DE (Driver Enable) low and RE (Receiver Enable) high for receive mode
    gpio_set_level(CONFIG_OSDP_RS485_DE_GPIO, 0);
    gpio_set_level(CONFIG_OSDP_RS485_RE_GPIO, 1);
}

// OSDP task for handling communication
static void osdp_task(void *pvParameters)
{
    uart_event_t event;
    uint8_t data[128];
    int len;
    
    ESP_LOGI(TAG, "OSDP task started");
    
    while (osdp_initialized) {
        if (xQueueReceive(osdp_rx_queue, (void *)&event, portMAX_DELAY)) {
            switch (event.type) {
                case UART_DATA:
                    len = uart_read_bytes(osdp_uart_port, data, event.size, portMAX_DELAY);
                    if (len > 0) {
                        ESP_LOGD(TAG, "Received %d bytes", len);
                        // TODO: Process OSDP data here
                    }
                    break;
                case UART_FIFO_OVF:
                case UART_BUFFER_FULL:
                    ESP_LOGW(TAG, "UART overflow or buffer full");
                    uart_flush_input(osdp_uart_port);
                    xQueueReset(osdp_rx_queue);
                    break;
                case UART_BREAK:
                    ESP_LOGW(TAG, "UART RX break");
                    break;
                case UART_PARITY_ERR:
                    ESP_LOGW(TAG, "UART parity error");
                    break;
                case UART_FRAME_ERR:
                    ESP_LOGW(TAG, "UART frame error");
                    break;
                default:
                    ESP_LOGW(TAG, "UART event type: %d", event.type);
                    break;
            }
        }
    }
    
    ESP_LOGI(TAG, "OSDP task ended");
    vTaskDelete(NULL);
}

// Initialize RS485 GPIO pins
static esp_err_t rs485_gpio_init(void)
{
    esp_err_t ret = ESP_OK;
    
    // Configure DE (Driver Enable) pin as output
    gpio_config_t de_config = {
        .pin_bit_mask = (1ULL << CONFIG_OSDP_RS485_DE_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&de_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure DE GPIO: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure RE (Receiver Enable) pin as output
    gpio_config_t re_config = {
        .pin_bit_mask = (1ULL << CONFIG_OSDP_RS485_RE_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&re_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure RE GPIO: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Set initial state to receive mode
    rs485_set_rx_mode();
    
    ESP_LOGI(TAG, "RS485 GPIO initialized - DE: GPIO%d, RE: GPIO%d", 
             CONFIG_OSDP_RS485_DE_GPIO, CONFIG_OSDP_RS485_RE_GPIO);
    
    return ESP_OK;
}

// Initialize UART for RS485 communication
static esp_err_t uart_init(void)
{
    esp_err_t ret = ESP_OK;
    
    // Determine baud rate from configuration
    int baud_rate = CONFIG_OSDP_BAUD_RATE;
    
    // UART configuration
    uart_config_t uart_config = {
        .baud_rate = baud_rate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    
    // Install UART driver
    ret = uart_driver_install(osdp_uart_port, 1024, 1024, 20, &osdp_rx_queue, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install UART driver: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Configure UART parameters
    ret = uart_param_config(osdp_uart_port, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure UART parameters: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Set UART pins
    ret = uart_set_pin(osdp_uart_port, 
                      CONFIG_OSDP_RS485_DI_GPIO,  // TX pin (DI)
                      CONFIG_OSDP_RS485_RO_GPIO,  // RX pin (RO)
                      UART_PIN_NO_CHANGE,         // RTS (not used)
                      UART_PIN_NO_CHANGE);        // CTS (not used)
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set UART pins: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "UART initialized - Port: %d, Baud: %d, TX: GPIO%d, RX: GPIO%d", 
             osdp_uart_port, baud_rate, CONFIG_OSDP_RS485_DI_GPIO, CONFIG_OSDP_RS485_RO_GPIO);
    
    return ESP_OK;
}

esp_err_t osdp_init(void)
{
    esp_err_t ret = ESP_OK;
    
    ESP_LOGI(TAG, "OSDP component initialization started");
    
    if (osdp_initialized) {
        ESP_LOGW(TAG, "OSDP already initialized");
        return ESP_OK;
    }
    
    // Initialize RS485 GPIO pins
    ret = rs485_gpio_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize RS485 GPIO");
        return ret;
    }
    
    // Initialize UART
    ret = uart_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize UART");
        return ret;
    }
    
    // Create OSDP task
    BaseType_t task_created = xTaskCreate(osdp_task, "osdp_task", 4096, NULL, 5, &osdp_task_handle);
    if (task_created != pdPASS) {
        ESP_LOGE(TAG, "Failed to create OSDP task");
        uart_driver_delete(osdp_uart_port);
        return ESP_FAIL;
    }
    
    osdp_initialized = true;
    
    ESP_LOGI(TAG, "OSDP component initialization completed successfully");
    ESP_LOGI(TAG, "OSDP Configuration:");
    ESP_LOGI(TAG, "  UART Port: %d", osdp_uart_port);
    ESP_LOGI(TAG, "  Baud Rate: %d", CONFIG_OSDP_BAUD_RATE);
    ESP_LOGI(TAG, "  PD Address: %d", CONFIG_OSDP_PD_ADDRESS);
    ESP_LOGI(TAG, "  RS485 Pins - DE: GPIO%d, DI: GPIO%d, RO: GPIO%d, RE: GPIO%d",
             CONFIG_OSDP_RS485_DE_GPIO, CONFIG_OSDP_RS485_DI_GPIO,
             CONFIG_OSDP_RS485_RO_GPIO, CONFIG_OSDP_RS485_RE_GPIO);
    
    return ESP_OK;
} 

esp_err_t osdp_deinit(void)
{
    ESP_LOGI(TAG, "OSDP component deinitialization started");
    
    if (!osdp_initialized) {
        ESP_LOGW(TAG, "OSDP not initialized");
        return ESP_OK;
    }
    
    // Stop the OSDP task
    osdp_initialized = false;
    
    if (osdp_task_handle != NULL) {
        vTaskDelete(osdp_task_handle);
        osdp_task_handle = NULL;
    }
    
    // Delete UART driver
    uart_driver_delete(osdp_uart_port);
    
    // Reset GPIO pins to default state
    gpio_reset_pin(CONFIG_OSDP_RS485_DE_GPIO);
    gpio_reset_pin(CONFIG_OSDP_RS485_RE_GPIO);
    
    ESP_LOGI(TAG, "OSDP component deinitialization completed");
    
    return ESP_OK;
}

esp_err_t osdp_get_status(void)
{
    if (!osdp_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    
    return ESP_OK;
}

