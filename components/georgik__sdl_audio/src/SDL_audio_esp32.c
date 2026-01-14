/*
 * SDL Audio ESP32 Backend Implementation
 * Based on ESP32-Quake audio implementation
 * Reference: /Users/georgik/projects/esp32-quake-hdmi/main/audio.c
 */

#include "SDL_audio_esp32.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
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
#define AUDIO_CHUNK_SIZE (SDL_AUDIO_BUFFER_SIZE / 8)  // 512 bytes with 4KB buffer
// Allocate from heap to save DRAM and allow PSRAM usage
static int16_t *audio_mix_buf = NULL;
static int16_t *audio_downsample_buf = NULL;  // Downsampled buffer for 22.05kHz output

// Audio callback from OpenTyrian (will be set by SDL_OpenAudioDevice)
static void (*sdl_audio_callback)(void *userdata, uint8_t *stream, int len) = NULL;
static void *sdl_audio_userdata = NULL;

// Audio task (following Quake's audio_task pattern)
static void audio_task(void *param)
{
    ESP_LOGI(TAG, "Audio task running (44.1kHz -> 22.05kHz mono)");

#if defined(CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV) && CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV
    if (!sdl_esp_audio_codec_dev) {
        ESP_LOGE(TAG, "Codec device not initialized");
        vTaskDelete(NULL);
        return;
    }
#endif

    // Test tone generator
    static uint32_t sample_count = 0;
    const uint32_t tone_frequency = 440;  // A4 note
    const uint32_t sample_rate = 44100;
    static int test_tone_counter = 0;

    while (1) {
        // Call OpenTyrian's audio callback to get mixed audio
        if (sdl_audio_callback && audio_initialized) {
            // Call the audio callback - gives us 44.1kHz mono int16 data
            sdl_audio_callback(sdl_audio_userdata, (uint8_t *)audio_mix_buf, AUDIO_CHUNK_SIZE);

            // Downsample from 44.1kHz to 22.05kHz (simple 2x decimation: drop every other sample)
            size_t samples_44k = AUDIO_CHUNK_SIZE / sizeof(int16_t);
            size_t samples_22k = samples_44k / 2;
            for (size_t i = 0; i < samples_22k; i++) {
                audio_downsample_buf[i] = audio_mix_buf[i * 2];  // Keep every other sample
            }

            // DEBUG: Check if callback produced any non-zero audio
            bool has_audio = false;
            int16_t max_sample = 0;
            for (size_t i = 0; i < samples_22k; i++) {
                if (audio_downsample_buf[i] != 0) {
                    has_audio = true;
                }
                if (abs(audio_downsample_buf[i]) > max_sample) {
                    max_sample = abs(audio_downsample_buf[i]);
                }
            }

            if (!has_audio && test_tone_counter++ < 100) {
                // Callback produced silence - inject test tone periodically
                // This happens every ~1 second for first few seconds
                ESP_LOGW(TAG, "Audio callback produced silence - injecting test tone (iteration %d, max_sample=%d)", test_tone_counter, max_sample);
                for (size_t i = 0; i < samples_22k; i++) {
                    float t = (float)sample_count / 22050;  // 22.05kHz for output
                    audio_downsample_buf[i] = (int16_t)(16000.0 * sin(2.0 * 3.14159 * tone_frequency * t));
                    sample_count++;
                }
            } else if (has_audio && test_tone_counter < 5) {
                ESP_LOGI(TAG, "Audio callback IS producing audio! max_sample=%d", max_sample);
                test_tone_counter = 100; // Stop showing test tone messages
            } else if (has_audio) {
                // Count how many times we get audio
                static int audio_frame_count = 0;
                if (++audio_frame_count % 2000 == 0) {
                    ESP_LOGI(TAG, "Audio frames with data: #%d, max_sample=%d", audio_frame_count, max_sample);
                }
            } else {
                // Silence during normal operation
                static int silence_count = 0;
                if (++silence_count % 2000 == 0) {
                    ESP_LOGW(TAG, "Audio callback producing silence: #%d", silence_count);
                }
            }

#if defined(CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV) && CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV
            // Write downsampled audio to codec (22.05kHz mono)
            size_t bytes_22k = samples_22k * sizeof(int16_t);
            esp_err_t ret = esp_codec_dev_write(
                sdl_esp_audio_codec_dev,
                (uint8_t *)audio_downsample_buf,
                bytes_22k
            );

            if (ret != ESP_OK) {
                ESP_LOGW(TAG, "Audio write failed: %s", esp_err_to_name(ret));
            } else {
                // Log successful write every ~2 seconds
                static int write_count = 0;
                if (++write_count % 2000 == 0) {
                    ESP_LOGI(TAG, "Audio write #%d: %zu bytes, has_audio=%d", write_count, bytes_22k, has_audio);
                }
            }
#endif
        } else {
            // No audio callback - generate test tone continuously
            if (test_tone_counter++ % 100 == 0) {
                ESP_LOGW(TAG, "No audio callback - generating test tone (iteration %d)", test_tone_counter);
            }
#if defined(CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV) && CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV
            // Generate 440Hz sine wave test tone (22.05kHz mono)
            size_t samples_22k = AUDIO_CHUNK_SIZE / sizeof(int16_t) / 2;
            for (size_t i = 0; i < samples_22k; i++) {
                float t = (float)sample_count / 22050;  // 22.05kHz output
                audio_downsample_buf[i] = (int16_t)(16000.0 * sin(2.0 * 3.14159 * tone_frequency * t));
                sample_count++;
            }

            size_t bytes_22k = samples_22k * sizeof(int16_t);
            esp_err_t ret = esp_codec_dev_write(
                sdl_esp_audio_codec_dev,
                (uint8_t *)audio_downsample_buf,
                bytes_22k
            );

            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Test tone write failed: %s", esp_err_to_name(ret));
            }
#endif
        }

        // No delay - keep audio pipeline running continuously
        // The ESP-BSP example writes continuously without delay
        // vTaskDelay(pdMS_TO_TICKS(1));
    }
}

bool SDL_ESP_Audio_Init(void)
{
    if (audio_initialized) {
        ESP_LOGW(TAG, "Audio already initialized");
        return true;
    }

#if defined(CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV) && CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV
    ESP_LOGI(TAG, "Initializing ESP32 audio (TX-only to save memory)");

    // Allocate audio mixing buffer from heap (prefer PSRAM)
    size_t mix_buf_samples = AUDIO_CHUNK_SIZE / sizeof(int16_t);
    audio_mix_buf = heap_caps_malloc(mix_buf_samples * sizeof(int16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!audio_mix_buf) {
        ESP_LOGW(TAG, "PSRAM malloc failed for audio mix buffer, trying regular malloc");
        audio_mix_buf = malloc(mix_buf_samples * sizeof(int16_t));
    }
    if (!audio_mix_buf) {
        ESP_LOGE(TAG, "Failed to allocate audio mix buffer");
        return false;
    }
    ESP_LOGI(TAG, "Audio mix buffer allocated: %zu bytes", mix_buf_samples * sizeof(int16_t));

    // Allocate downsampled buffer (half size for 22.05kHz output)
    size_t downsample_samples = mix_buf_samples / 2;
    audio_downsample_buf = heap_caps_malloc(downsample_samples * sizeof(int16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!audio_downsample_buf) {
        ESP_LOGW(TAG, "PSRAM malloc failed for downsample buffer, trying regular malloc");
        audio_downsample_buf = malloc(downsample_samples * sizeof(int16_t));
    }
    if (!audio_downsample_buf) {
        ESP_LOGE(TAG, "Failed to allocate downsample buffer");
        free(audio_mix_buf);
        audio_mix_buf = NULL;
        return false;
    }
    ESP_LOGI(TAG, "Audio downsample buffer allocated: %zu bytes", downsample_samples * sizeof(int16_t));

    // Use TX-only initialization to save DMA memory
    // This allocates only the TX channel, not RX (we don't need audio input)
    extern esp_codec_dev_handle_t bsp_audio_codec_speaker_init_tx_only(void);
    sdl_esp_audio_codec_dev = bsp_audio_codec_speaker_init_tx_only();

    if (!sdl_esp_audio_codec_dev) {
        ESP_LOGE(TAG, "Failed to initialize speaker codec");
        free(audio_mix_buf);
        audio_mix_buf = NULL;
        return false;
    }

    // Open the codec device for playback (22.05kHz mono)
    // MCLK_MULTIPLE_384 is required for ES8311 codec at 22.05kHz
    esp_codec_dev_sample_info_t fs = {
        .sample_rate = 22050,
        .channel = 1,
        .bits_per_sample = 16,
        .mclk_multiple = I2S_MCLK_MULTIPLE_384,  // Required for ES8311 at 22.05kHz
    };
    esp_err_t ret = esp_codec_dev_open(sdl_esp_audio_codec_dev, &fs);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open codec device: %s", esp_err_to_name(ret));
        esp_codec_dev_delete(sdl_esp_audio_codec_dev);
        sdl_esp_audio_codec_dev = NULL;
        free(audio_mix_buf);
        audio_mix_buf = NULL;
        return false;
    }

    // Set initial volume
    esp_codec_dev_set_out_vol(sdl_esp_audio_codec_dev, current_volume);
    esp_codec_dev_set_out_mute(sdl_esp_audio_codec_dev, 0);

    ESP_LOGI(TAG, "Audio initialized: 44.1kHz -> 22.05kHz mono (2x downsampling)");

    // Create audio task with larger stack for OPL emulator
    // OPL emulator (adlib_getsample) needs significant stack space
    // Increased from 4KB to 12KB to prevent stack overflow
    BaseType_t ret_val = xTaskCreatePinnedToCore(
        audio_task,
        "sdl_audio",
        12 * 1024,  // 12KB stack for OPL emulator + audio processing
        NULL,
        7,  // Priority
        NULL,
        1   // Core 1
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

    // Free audio buffers
    if (audio_mix_buf) {
        free(audio_mix_buf);
        audio_mix_buf = NULL;
    }
    if (audio_downsample_buf) {
        free(audio_downsample_buf);
        audio_downsample_buf = NULL;
    }

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
