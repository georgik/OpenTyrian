#include "opentyr.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"  // Optional: Remove if no FreeRTOS features are used
#include "keyboard.h"
#include "SDL3/SDL_esp-idf.h"

// Function to check and print memory usage
void check_memory_main() {
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t free_dram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

    printf("Available PSRAM: %zu, DRAM: %zu\n", free_psram, free_dram);
}

// Thread to periodically check memory usage
void* memory_check_thread(void *args)
{
    while (1) {
        check_memory_main();
        usleep(1000 * 1000);  // Sleep for 1000 ms (1 second)
    }
    return NULL;
}

// Thread for keyboard processing
void* keyboard_thread(void *args)
{
    while (1) {
        process_keyboard();
        usleep(50 * 1000);  // Sleep for 50 ms
    }
    return NULL;
}

// Thread to run Tyrian
void* tyrian_thread(void* args) {

#ifdef CONFIG_IDF_TARGET_ESP32P4
    printf("USB - Keyboard initialization\n");

    // Temporary solution to transport scaling factor directly to framebuffer
    // This will invoke PPA
    set_scale_factor(3, 3.0);
    init_keyboard();

    // Create the keyboard processing thread
    pthread_t keyboard_pthread;
    pthread_attr_t keyboard_attr;
    pthread_attr_init(&keyboard_attr);
    pthread_attr_setstacksize(&keyboard_attr, 8192);  // Set stack size for keyboard thread

    int ret = pthread_create(&keyboard_pthread, &keyboard_attr, keyboard_thread, NULL);
    if (ret != 0) {
        printf("Failed to create keyboard thread: %d\n", ret);
        return NULL;
    }
    pthread_detach(keyboard_pthread);

    // Delay required for the keyboard to initialize and allocate the DMA capable memory
    usleep(500 * 1000);  // Sleep for 500 ms
#endif

    char *argv[] = {"opentyrian", NULL};
    main(1, argv);
    return NULL;
}

void app_main(void) {
    printf("OpenTyrian initialization...\n");

    pthread_t sdl_pthread, memory_check_pthread;

    // Initialize SDL thread
    pthread_attr_t sdl_attr;
    pthread_attr_init(&sdl_attr);
    pthread_attr_setstacksize(&sdl_attr, 32000);  // Set stack size for SDL thread

    int ret = pthread_create(&sdl_pthread, &sdl_attr, tyrian_thread, NULL);
    if (ret != 0) {
        printf("Failed to create SDL thread: %d\n", ret);
        return;
    }
    pthread_detach(sdl_pthread);

    // Initialize memory check thread
    pthread_attr_t memory_attr;
    pthread_attr_init(&memory_attr);
    pthread_attr_setstacksize(&memory_attr, 8192);  // Set stack size for memory check thread

    ret = pthread_create(&memory_check_pthread, &memory_attr, memory_check_thread, NULL);
    if (ret != 0) {
        printf("Failed to create memory check thread: %d\n", ret);
        return;
    }
    pthread_detach(memory_check_pthread);
}
