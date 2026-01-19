/**
 * @file esp_bsp_sdl_esp_bsp_devkit.c
 * @brief ESP BSP DevKit specific implementation for ESP-BSP SDL abstraction layer
 * Uses esp_bsp_devkit for basic DevKit support (LEDs, buttons, no display)
 * Note: This creates a virtual display for SDL since DevKit BSP has no display
 */

#include "esp_bsp_devkit.h"
#include "esp_bsp_sdl.h"
#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"

// SDL pixel format constants - using direct values to avoid SDL dependency
#define SDL_PIXELFORMAT_RGB565 0x15151002u

// Virtual display dimensions for DevKit (no physical display)
#define VIRTUAL_DISPLAY_WIDTH 240
#define VIRTUAL_DISPLAY_HEIGHT 320

static const char *TAG = "esp_bsp_sdl_esp_bsp_devkit";

esp_err_t esp_bsp_sdl_board_init(esp_bsp_sdl_display_config_t *config,
                                 esp_lcd_panel_handle_t *panel_handle,
                                 esp_lcd_panel_io_handle_t *panel_io_handle)
{
    ESP_LOGI(TAG, "Initializing ESP BSP DevKit (LEDs/Buttons, No Display)");

    if(!config || !panel_handle || !panel_io_handle) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ESP_OK;

    // DevKit BSP has no display - create virtual display configuration for SDL
    config->width = VIRTUAL_DISPLAY_WIDTH;
    config->height = VIRTUAL_DISPLAY_HEIGHT;
    config->pixel_format = SDL_PIXELFORMAT_RGB565;
    config->max_transfer_sz = (config->width * config->height) * sizeof(uint16_t);
    config->has_touch = false;  // DevKit BSP doesn't support touch

    ESP_LOGI(TAG, "Virtual display configured: %dx%d (no physical display)", config->width, config->height);

    // Initialize DevKit BSP features (LEDs, buttons, file systems)
    ESP_LOGI(TAG, "Initializing DevKit BSP features...");

    // Initialize LEDs if configured
#ifdef CONFIG_BSP_DEVKIT_LED_ENABLED
    ret = bsp_leds_init();
    if(ret == ESP_OK) {
        ESP_LOGI(TAG, "LEDs initialized successfully");
    } else {
        ESP_LOGW(TAG, "LED initialization failed: %s", esp_err_to_name(ret));
    }
#endif

    // Initialize buttons if configured
#ifdef CONFIG_BSP_DEVKIT_BUTTON_ENABLED
    ret = bsp_iot_button_create();
    if(ret == ESP_OK) {
        ESP_LOGI(TAG, "Buttons initialized successfully");
    } else {
        ESP_LOGW(TAG, "Button initialization failed: %s", esp_err_to_name(ret));
    }
#endif

    // Initialize file systems if configured
#ifdef CONFIG_BSP_DEVKIT_SPIFFS_ENABLED
    ret = bsp_spiffs_mount();
    if(ret == ESP_OK) {
        ESP_LOGI(TAG, "SPIFFS mounted successfully");
    } else {
        ESP_LOGW(TAG, "SPIFFS mount failed: %s", esp_err_to_name(ret));
    }
#endif

#ifdef CONFIG_BSP_DEVKIT_USD_ENABLED
    // Note: uSD card initialization would be done here if configured
    ESP_LOGI(TAG, "uSD card support configured (mount on demand)");
#endif

    // No physical display handles for DevKit
    *panel_handle = NULL;
    *panel_io_handle = NULL;

    ESP_LOGI(TAG, "ESP BSP DevKit initialized (virtual display: %dx%d)", config->width, config->height);

    return ESP_OK;
}

esp_err_t esp_bsp_sdl_board_backlight_on(void)
{
    ESP_LOGW(TAG, "DevKit has no backlight - ignoring backlight_on request");
    return ESP_OK;  // Return success since this is expected for DevKit
}

int esp_bsp_sdl_board_backlight_off(void)
{
    ESP_LOGW(TAG, "DevKit has no backlight - ignoring backlight_off request");
    return ESP_OK;  // Return success since this is expected for DevKit
}

esp_err_t esp_bsp_sdl_board_display_on_off(bool enable)
{
    ESP_LOGD(TAG, "%s virtual display (no physical display on DevKit)", enable ? "Enabling" : "Disabling");
    return ESP_OK;  // Always succeed for virtual display
}

esp_err_t esp_bsp_sdl_board_touch_init(void)
{
    ESP_LOGI(TAG, "DevKit has no touch interface");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t esp_bsp_sdl_board_touch_read(esp_bsp_sdl_touch_info_t *touch_info)
{
    return ESP_ERR_NOT_SUPPORTED;  // DevKit has no touch
}

const char *esp_bsp_sdl_board_get_name(void)
{
    return "ESP BSP DevKit (LEDs/Buttons)";
}

esp_err_t esp_bsp_sdl_board_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing ESP BSP DevKit");

    // Clean up DevKit BSP resources if needed
#ifdef CONFIG_BSP_DEVKIT_SPIFFS_ENABLED
    bsp_spiffs_unmount();
#endif

    return ESP_OK;
}