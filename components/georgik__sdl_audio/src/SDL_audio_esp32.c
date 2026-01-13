/*
 * SDL Audio ESP32 Backend Implementation
 * Based on ESP32-Quake audio implementation
 * Reference: /Users/georgik/projects/esp32-quake-hdmi/main/audio.c
 */

#include "SDL_audio_esp32.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

// Conditional BSP includes (only available on ESP32-P4 Function EV Board)
#if defined(CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV) && CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV
#include "esp_codec_dev.h"
#include "bsp/esp-bsp.h"
#endif

static const char *TAG = "SDL_audio_esp32";

// Audio state (following Quake pattern)
esp_codec_dev_handle_t sdl_esp_audio_codec_dev = NULL;
static bool audio_initialized = false;
static int current_volume = 50;

// DMA buffer (following Quake's dma_buffer pattern)
#define AUDIO_CHUNK_SIZE (SDL_AUDIO_BUFFER_SIZE / 8)  // 2KB chunks
static int16_t audio_mix_buf[AUDIO_CHUNK_SIZE / sizeof(int16_t)];

// Audio callback from OpenTyrian (will be set by SDL_OpenAudioDevice)
static void (*sdl_audio_callback)(void *userdata, uint8_t *stream, int len) = NULL;
static void *sdl_audio_userdata = NULL;

// Audio task (following Quake's audio_task pattern)
static void audio_task(void *param)
{
    ESP_LOGI(TAG, "Audio task running");

#if defined(CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV) && CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV
    if (!sdl_esp_audio_codec_dev) {
        ESP_LOGE(TAG, "Codec device not initialized");
        vTaskDelete(NULL);
        return;
    }

    // Downsample buffer: BSP uses 22.05kHz mono, OpenTyrian provides 44.1kHz stereo
    // We need to convert: stereo -> mono and downsample 2:1
    // Allocate buffer for downsampled mono audio
    // 44.1kHz stereo (2 channels) -> 22.05kHz mono (1 channel) = 4:1 reduction in samples
    size_t downsampled_samples = (AUDIO_CHUNK_SIZE / sizeof(int16_t)) / 4;
    int16_t *mono_buf = malloc(downsampled_samples * sizeof(int16_t));
    if (!mono_buf) {
        ESP_LOGE(TAG, "Failed to allocate mono buffer");
        vTaskDelete(NULL);
        return;
    }
#endif

    while (1) {
        // Call OpenTyrian's audio callback to get mixed audio
        if (sdl_audio_callback && audio_initialized) {
            // Call the audio callback - gives us 44.1kHz stereo int16 data
            sdl_audio_callback(sdl_audio_userdata, (uint8_t *)audio_mix_buf, AUDIO_CHUNK_SIZE);

#if defined(CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV) && CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV
            // Downsample 44.1kHz stereo to 22.05kHz mono
            // For every 4 stereo samples (8 int16), take the average of left+right channels
            // This gives us 1 mono sample
            size_t stereo_samples = AUDIO_CHUNK_SIZE / sizeof(int16_t);
            for (size_t i = 0; i < downsampled_samples; i++) {
                // Take 2 stereo samples (4 int16 values: L1,R1,L2,R2)
                // Average them to get 1 mono sample
                int32_t sum = 0;
                for (int j = 0; j < 4; j++) {
                    sum += audio_mix_buf[i * 4 + j];
                }
                mono_buf[i] = (int16_t)(sum / 4);
            }

            // Write downsampled mono audio to codec
            esp_err_t ret = esp_codec_dev_write(
                sdl_esp_audio_codec_dev,
                (uint8_t *)mono_buf,
                downsampled_samples * sizeof(int16_t)
            );

            if (ret != ESP_OK) {
                // ESP_LOGW(TAG, "Audio write failed: %s", esp_err_to_name(ret));
            }
#endif
        } else {
            // No audio callback, write silence
#if defined(CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV) && CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV
            memset(mono_buf, 0, downsampled_samples * sizeof(int16_t));
            esp_codec_dev_write(sdl_esp_audio_codec_dev, (uint8_t *)mono_buf, downsampled_samples * sizeof(int16_t));
#endif
        }

        // Small delay to prevent watchdog
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

bool SDL_ESP_Audio_Init(void)
{
    if (audio_initialized) {
        ESP_LOGW(TAG, "Audio already initialized");
        return true;
    }

#if defined(CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV) && CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV
    ESP_LOGI(TAG, "Initializing ESP32 audio (following Quake pattern)");

    // Initialize speaker codec - this will call bsp_audio_init(NULL) internally
    // which uses the default working configuration (22.05kHz mono)
    // Note: We don't call esp_codec_dev_open() to avoid DMA reallocation failure
    sdl_esp_audio_codec_dev = bsp_audio_codec_speaker_init();
    if (!sdl_esp_audio_codec_dev) {
        ESP_LOGE(TAG, "Failed to initialize speaker codec");
        return false;
    }

    // Set initial volume
    esp_codec_dev_set_out_vol(sdl_esp_audio_codec_dev, current_volume);
    esp_codec_dev_set_out_mute(sdl_esp_audio_codec_dev, 0);

    ESP_LOGI(TAG, "Audio initialized with BSP default configuration");
    ESP_LOGI(TAG, "Note: Audio will be resampled from 44.1kHz stereo to 22.05kHz mono");

    // Create audio task (following Quake's xTaskCreatePinnedToCore)
    BaseType_t ret_val = xTaskCreatePinnedToCore(
        audio_task,
        "sdl_audio",
        4096,
        NULL,
        7,  // Priority (same as Quake)
        NULL,
        1   // Core 1 (same as Quake)
    );

    if (ret_val != pdPASS) {
        ESP_LOGE(TAG, "Failed to create audio task");
        return false;
    }

    audio_initialized = true;
    ESP_LOGI(TAG, "ESP32 audio backend ready");
    return true;
#else
    ESP_LOGI(TAG, "Audio not supported on this board (BSP not available)");
    return false;
#endif
}

void SDL_ESP_Audio_Shutdown(void)
{
    if (!audio_initialized) {
        return;
    }

    ESP_LOGI(TAG, "Shutting down audio");
    audio_initialized = false;

#if defined(CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV) && CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV
    if (sdl_esp_audio_codec_dev) {
        esp_codec_dev_close(sdl_esp_audio_codec_dev);
        esp_codec_dev_delete(sdl_esp_audio_codec_dev);
        sdl_esp_audio_codec_dev = NULL;
    }
#endif

    ESP_LOGI(TAG, "Audio shutdown complete");
}

void SDL_ESP_Audio_SetVolume(int volume)
{
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;

    current_volume = volume;

#if defined(CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV) && CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV
    if (sdl_esp_audio_codec_dev && audio_initialized) {
        esp_codec_dev_set_out_vol(sdl_esp_audio_codec_dev, volume);
        ESP_LOGI(TAG, "Volume set to %d", volume);
    }
#endif
}

bool SDL_ESP_Audio_IsInitialized(void)
{
    return audio_initialized;
}

// SDL audio callback registration (called from SDL_OpenAudioDevice)
void SDL_ESP_RegisterAudioCallback(void (*callback)(void *, uint8_t *, int), void *userdata)
{
    sdl_audio_callback = callback;
    sdl_audio_userdata = userdata;
    ESP_LOGI(TAG, "Audio callback registered");
}
