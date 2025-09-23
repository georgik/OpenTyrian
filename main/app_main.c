#include "opentyr.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "keyboard.h"
#include "SDL3/SDL.h"
#ifdef CONFIG_IDF_TARGET_ESP32P4
#    include "SDL3/SDL_esp-idf.h"
#endif


// Function to check and print memory usage
void check_memory_main()
{
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t free_dram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

    printf("Available PSRAM: %zu, DRAM: %zu\n", free_psram, free_dram);
}

// Thread to periodically check memory usage - reduced frequency for better performance
void *memory_thread(void *args)
{
    while(1) {
        check_memory_main();
        usleep(30000 * 1000);  // Check every 30 seconds instead of 10
    }
    return NULL;
}

// Thread for keyboard processing - optimized for Core 1
void *keyboard_thread(void *args)
{
    while(1) {
        process_keyboard();
        usleep(50 * 1000);  // Sleep for 50 ms
    }
    return NULL;
}

// Function to setup SDL scaling and rotation based on configuration
void setup_sdl_scaling(SDL_Renderer *renderer)
{
    printf("Setting up SDL scaling and rotation...\n");

#ifdef CONFIG_IDF_TARGET_ESP32P4
    // Rotation and scale factor are now set in video.c before framebuffer creation
    // This avoids PPA buffer allocation timing issues
#endif

    if(!renderer) {
        printf("No renderer available, but rotation configured for ESP-IDF framebuffer\n");
        return;
    }

#ifdef CONFIG_OPENTYRIAN_SCALE_LOGICAL_PRESENTATION
    // Use SDL logical presentation mode
    SDL_RendererLogicalPresentation presentation_mode;

#    ifdef CONFIG_OPENTYRIAN_LOGICAL_LETTERBOX
    presentation_mode = SDL_LOGICAL_PRESENTATION_LETTERBOX;
#    elif CONFIG_OPENTYRIAN_LOGICAL_OVERSCAN
    presentation_mode = SDL_LOGICAL_PRESENTATION_OVERSCAN;
#    elif CONFIG_OPENTYRIAN_LOGICAL_STRETCH
    presentation_mode = SDL_LOGICAL_PRESENTATION_STRETCH;
#    else
    presentation_mode = SDL_LOGICAL_PRESENTATION_LETTERBOX;
#    endif

    if(!SDL_SetRenderLogicalPresentation(
           renderer, CONFIG_OPENTYRIAN_LOGICAL_WIDTH, CONFIG_OPENTYRIAN_LOGICAL_HEIGHT, presentation_mode)) {
        printf("Failed to set logical presentation: %s\n", SDL_GetError());
    } else {
        printf("SDL logical presentation enabled: %dx%d\n",
               CONFIG_OPENTYRIAN_LOGICAL_WIDTH,
               CONFIG_OPENTYRIAN_LOGICAL_HEIGHT);
    }

#elif defined(CONFIG_OPENTYRIAN_SCALE_FACTOR_INT) && CONFIG_OPENTYRIAN_SCALE_FACTOR_INT > 1
    // Skip SDL render scaling on ESP32-P4 - let PPA handle rotation+scaling in hardware
#    ifdef CONFIG_IDF_TARGET_ESP32P4
    printf("Skipping SDL render scaling - using PPA hardware scaling (%dx)\n", CONFIG_OPENTYRIAN_SCALE_FACTOR_INT);
#    else
    // Use SDL render scaling on other targets
    float render_scale_factor = (float) CONFIG_OPENTYRIAN_SCALE_FACTOR_INT;

    if(!SDL_SetRenderScale(renderer, render_scale_factor, render_scale_factor)) {
        printf("Failed to set render scale: %s\n", SDL_GetError());
    } else {
        printf("SDL render scaling enabled: %.1fx (board-optimized)\n", render_scale_factor);
#        ifdef CONFIG_OPENTYRIAN_DEBUG_SCALING
        printf("Scale factor from Kconfig: %d\n", CONFIG_OPENTYRIAN_SCALE_FACTOR_INT);
#        endif
    }
#    endif
#else
    printf("No scaling applied - using native resolution\n");
#endif
}

// Thread to run Tyrian
void *tyrian_thread(void *args)
{
    printf("Starting Tyrian game loop on Core 0\n");

#ifdef CONFIG_IDF_TARGET_ESP32P4
    printf("USB - Keyboard initialization\n");
    init_keyboard();

    // Delay required for the keyboard to initialize and allocate the DMA capable memory
    usleep(500 * 1000);  // Sleep for 500 ms
    printf("Keyboard initialization complete\n");
#endif

    char *argv[] = {"opentyrian", NULL};
    if(main(1, argv) != 0) {
        printf("Error in main\n");
        return NULL;
    }

    printf("Tyrian game exited cleanly\n");
    return NULL;
}

void app_main(void)
{
    printf("OpenTyrian initialization (SDL-compatible)...\n");

    printf("Hardware acceleration: %s\n", CONFIG_OPENTYRIAN_ENABLE_HARDWARE_ACCELERATION ? "enabled" : "disabled");

    // Create main game thread for SDL compatibility
    pthread_t main_thread_id;
    pthread_attr_t main_attr;
    pthread_attr_init(&main_attr);
    pthread_attr_setstacksize(&main_attr, 32000);

    if(pthread_create(&main_thread_id, &main_attr, tyrian_thread, NULL) != 0) {
        printf("Error creating main thread\n");
        return;
    }

    pthread_attr_destroy(&main_attr);
    printf("Main game thread created\n");

#ifdef CONFIG_IDF_TARGET_ESP32P4
    // Create keyboard processing thread for USB input handling
    pthread_t keyboard_thread_id;
    pthread_attr_t keyboard_attr;
    pthread_attr_init(&keyboard_attr);
    pthread_attr_setstacksize(&keyboard_attr, 8192);

    if(pthread_create(&keyboard_thread_id, &keyboard_attr, keyboard_thread, NULL) != 0) {
        printf("Warning: Failed to create keyboard processing thread\n");
        // Continue without dedicated keyboard thread
    } else {
        pthread_attr_destroy(&keyboard_attr);
        printf("Keyboard processing thread created\n");
    }
#endif

    // Wait for the main thread to complete
    pthread_join(main_thread_id, NULL);

    printf("Tyrian game completed\n");
}
