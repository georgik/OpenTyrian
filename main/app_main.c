#include "opentyr.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "keyboard.h"
#include "SDL3/SDL_esp-idf.h"

// Task to run Tyrian
void tyrianTask(void *pvParameters)
{
    char *argv[] = {"opentyrian", NULL};
    main(1, argv);
}

// Function to check and print memory usage
void check_memory_main() {
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t free_dram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

    printf("Available PSRAM: %zu, DRAM: %zu\n", free_psram, free_dram);
}

// Task to periodically check memory usage
void memoryCheckTask(void *pvParameters)
{
    while (1) {
        check_memory_main();
        vTaskDelay(pdMS_TO_TICKS(1000));  // Delay for 1000 ms (1 second)
    }
}

void keyboardTask(void *pvParameters)
{
    while (1) {
        process_keyboard();
        vTaskDelay(pdMS_TO_TICKS(50));  // Delay for 1000 ms (1 second)
    }
}


// Application main entry point
void app_main(void)
{
    // Create a task to check memory every second
    // xTaskCreatePinnedToCore(&memoryCheckTask, "memoryCheckTask", 2048, NULL, 1, NULL, 0);

    printf("USB - Keyboard initialization\n");

#ifdef CONFIG_IDF_TARGET_ESP32P4
    // Temporary solution to transport scaling factor directly to framebuffer
    // This will invoke PPA
    set_scale_factor(3, 3.0);
    init_keyboard();

    xTaskCreatePinnedToCore(&keyboardTask, "keyboardTask", 8912, NULL, 1, NULL, 0);
    // Delay required for the keyboard to initialize and allocate the DMA capable memory
    vTaskDelay(pdMS_TO_TICKS(500));
#endif

    printf("OpenTyrian initialization...\n");
    // Create a task for the Tyrian game
    xTaskCreatePinnedToCore(&tyrianTask, "tyrianTask", 32000, NULL, 5, NULL, 0);


}
