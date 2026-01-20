/**
 * @file esp_bsp_sdl_esp_bsp_generic.c
 * @brief ESP BSP Generic specific implementation for ESP-BSP SDL abstraction layer
 * Uses esp_bsp_generic for configurable DevKit + Display support
 * Supports any ESP32 DevKit with custom display/touch configuration via menuconfig
 */

#include "esp_bsp_generic.h"
#include "esp_bsp_sdl.h"
#include "esp_err.h"
#include "esp_log.h"
#include "sdkconfig.h"

// SDL pixel format constants - using direct values to avoid SDL dependency
#define SDL_PIXELFORMAT_RGB565 0x15151002u

static const char *TAG = "esp_bsp_sdl_esp_bsp_generic";
static esp_lcd_panel_handle_t s_panel_handle = NULL;
static esp_lcd_panel_io_handle_t s_panel_io_handle = NULL;

esp_err_t esp_bsp_sdl_board_init(esp_bsp_sdl_display_config_t *config,
                                 esp_lcd_panel_handle_t *panel_handle,
                                 esp_lcd_panel_io_handle_t *panel_io_handle)
{
    ESP_LOGI(TAG, "Initializing ESP BSP Generic (Configurable DevKit)");

    if(!config || !panel_handle || !panel_io_handle) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = ESP_OK;

    // Step 1: Get display configuration from BSP Generic
    // The actual resolution and settings are configured via menuconfig
#ifdef CONFIG_BSP_GENERIC_DISPLAY_ENABLED
    // Read configuration from BSP Generic settings
    config->width = CONFIG_BSP_GENERIC_DISPLAY_WIDTH;
    config->height = CONFIG_BSP_GENERIC_DISPLAY_HEIGHT;
    config->pixel_format = SDL_PIXELFORMAT_RGB565;
    config->max_transfer_sz = (config->width * config->height) * sizeof(uint16_t);
    config->has_touch = CONFIG_BSP_GENERIC_TOUCH_ENABLED;

    ESP_LOGI(TAG,
             "Display configured: %dx%d, Touch: %s",
             config->width,
             config->height,
             config->has_touch ? "enabled" : "disabled");

    // Step 2: Initialize BSP Generic display
    ESP_LOGI(TAG, "Initializing display via BSP Generic...");
    ret = bsp_display_new(NULL, &s_panel_handle, &s_panel_io_handle);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize BSP Generic display: %s", esp_err_to_name(ret));
        return ret;
    }

    // Step 3: Turn on the display
    ESP_LOGI(TAG, "Enabling display...");
    ret = esp_lcd_panel_disp_on_off(s_panel_handle, true);
    if(ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to turn on display: %s", esp_err_to_name(ret));
        return ret;
    }

    // Step 4: Initialize backlight if available
    ESP_LOGI(TAG, "Initializing backlight control...");
    ret = bsp_display_brightness_init();
    if(ret == ESP_OK) {
        ret = bsp_display_backlight_on();
        if(ret != ESP_OK) {
            ESP_LOGW(TAG, "Backlight control failed: %s", esp_err_to_name(ret));
        }
    } else {
        ESP_LOGI(TAG, "No backlight control configured");
    }

    *panel_handle = s_panel_handle;
    *panel_io_handle = s_panel_io_handle;

#else
    // No display configured - this is unusual for SDL applications but supported
    ESP_LOGW(TAG, "No display configured in BSP Generic - SDL will use virtual display");
    config->width = 240;  // Default fallback size
    config->height = 320;
    config->pixel_format = SDL_PIXELFORMAT_RGB565;
    config->max_transfer_sz = (config->width * config->height) * sizeof(uint16_t);
    config->has_touch = false;

    *panel_handle = NULL;
    *panel_io_handle = NULL;
#endif

    ESP_LOGI(TAG, "ESP BSP Generic initialized: %dx%d", config->width, config->height);

    return ESP_OK;
}

esp_err_t esp_bsp_sdl_board_backlight_on(void)
{
    ESP_LOGI(TAG, "Turning backlight on");
    return bsp_display_backlight_on();
}

int esp_bsp_sdl_board_backlight_off(void)
{
    ESP_LOGI(TAG, "Turning backlight off");
    return bsp_display_backlight_off();
}

esp_err_t esp_bsp_sdl_board_display_on_off(bool enable)
{
    ESP_LOGD(TAG, "%s display", enable ? "Enabling" : "Disabling");
    if(s_panel_handle) {
        return esp_lcd_panel_disp_on_off(s_panel_handle, enable);
    }
    return ESP_ERR_INVALID_STATE;
}

esp_err_t esp_bsp_sdl_board_touch_init(void)
{
#ifdef CONFIG_BSP_GENERIC_TOUCH_ENABLED
    ESP_LOGI(TAG, "Initializing touch interface");

    esp_err_t ret = bsp_touch_new(NULL, NULL);
    if(ret == ESP_OK) {
        ESP_LOGI(TAG, "Touch interface initialized successfully");
        return ESP_OK;
    } else {
        ESP_LOGW(TAG, "Touch initialization failed: %s", esp_err_to_name(ret));
        return ret;
    }
#else
    ESP_LOGI(TAG, "Touch not configured in BSP Generic");
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

esp_err_t esp_bsp_sdl_board_touch_read(esp_bsp_sdl_touch_info_t *touch_info)
{
#ifdef CONFIG_BSP_GENERIC_TOUCH_ENABLED
    if(!touch_info) {
        return ESP_ERR_INVALID_ARG;
    }

    // TODO: Implement touch reading for BSP Generic
    // This would need to be implemented based on the actual touch driver configured
    touch_info->pressed = false;
    touch_info->x = 0;
    touch_info->y = 0;

    return ESP_ERR_NOT_SUPPORTED;  // Not yet implemented
#else
    return ESP_ERR_NOT_SUPPORTED;
#endif
}

const char *esp_bsp_sdl_board_get_name(void)
{
    return "ESP BSP Generic (Configurable)";
}

esp_err_t esp_bsp_sdl_board_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing ESP BSP Generic");

    if(s_panel_handle) {
        s_panel_handle = NULL;
    }

    if(s_panel_io_handle) {
        s_panel_io_handle = NULL;
    }

    return ESP_OK;
}