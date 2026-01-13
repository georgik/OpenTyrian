/*
 * SDL Audio ESP32 Backend
 * ESP-IDF wrapper for SDL3 audio on ESP32-P4
 * Based on ESP32-Quake audio implementation
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// Forward declaration for codec device handle (opaque pointer when BSP unavailable)
#if defined(CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV) && CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV
#include "esp_codec_dev.h"
#else
typedef void *esp_codec_dev_handle_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Audio configuration
#define SDL_AUDIO_SAMPLE_RATE     44100
#define SDL_AUDIO_CHANNELS        2  // Stereo (mono converted to stereo)
#define SDL_AUDIO_BITS_PER_SAMPLE 16
#define SDL_AUDIO_BUFFER_SIZE     (16*1024)  // 16KB DMA buffer

// Audio device handle (similar to Quake's spk_codec_dev)
extern esp_codec_dev_handle_t sdl_esp_audio_codec_dev;

// Audio initialization (following Quake's SNDDMA_Init pattern)
bool SDL_ESP_Audio_Init(void);
void SDL_ESP_Audio_Shutdown(void);

// Audio playback (following Quake's audio_task pattern)
void SDL_ESP_Audio_Write(int16_t *buffer, size_t len);

// Volume control
void SDL_ESP_Audio_SetVolume(int volume);  // 0-100

// Check if audio is initialized
bool SDL_ESP_Audio_IsInitialized(void);

// SDL audio callback registration (called from SDL_OpenAudioDevice)
void SDL_ESP_RegisterAudioCallback(void (*callback)(void *, uint8_t *, int), void *userdata);

#ifdef __cplusplus
}
#endif
