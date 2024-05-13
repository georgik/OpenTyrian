#include "SDL_system.h"

struct SDL_mutex
{
    pthread_mutex_t id;
#if FAKE_RECURSIVE_MUTEX
    int recursive;
    pthread_t owner;
#endif
};

void SDL_ClearError(void)
{

}

void SDL_Delay(Uint32 ms)
{
    const TickType_t xDelay = ms / portTICK_PERIOD_MS;
    vTaskDelay( xDelay );
}

char *SDL_GetError(void)
{
    return (char *)"";
}

int SDL_Init(Uint32 flags)
{
    if(flags == SDL_INIT_VIDEO)
        SDL_InitSubSystem(flags);
    return 0;
}

void SDL_QuitFS(void) {
    esp_vfs_littlefs_unregister("/spiffs");
    printf("Filesystem unmounted\n");
}

void SDL_Quit(void)
{
    SDL_QuitFS();
}

// void SDL_InitSD(void)
// {
//     printf("Initialising SD Card\n");
// #if 0
// 	sdmmc_host_t host = SDSPI_HOST_DEFAULT();
//     host.command_timeout_ms = 3000;
//     host.max_freq_khz = SDMMC_FREQ_DEFAULT;
//     // https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/spi_master.html
//     host.slot = CONFIG_HW_SD_PIN_NUM_MISO == 0 ? VSPI_HOST : HSPI_HOST;
//     sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
//     slot_config.gpio_miso = CONFIG_HW_SD_PIN_NUM_MISO;
//     slot_config.gpio_mosi = CONFIG_HW_SD_PIN_NUM_MOSI;
//     slot_config.gpio_sck  = CONFIG_HW_SD_PIN_NUM_CLK;
//     slot_config.gpio_cs   = CONFIG_HW_SD_PIN_NUM_CS;
// 	//slot_config.dma_channel = 1; //2

// #else
// 	sdmmc_host_t host = SDMMC_HOST_DEFAULT();
// 	// host.flags = SDMMC_HOST_FLAG_1BIT;
// 	//host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
// 	host.command_timeout_ms=1500;
// 	sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
// 	slot_config.width = 1;
// #endif
//     esp_vfs_fat_sdmmc_mount_config_t mount_config;
//     memset(&mount_config, 0, sizeof(mount_config));
//     mount_config.format_if_mount_failed = false;
//     mount_config.max_files = 5;

// 	sdmmc_card_t* card;
//     SDL_LockDisplay();
//     SDL_Delay(200);
//     esp_err_t err = esp_vfs_fat_sdmmc_mount("/sd", &host, &slot_config, &mount_config, &card);
//     if(err != ESP_OK)  // Wait and try again
//         for(int i = 0; i < 10; i++)
//         {
//             SDL_Delay(500);
//             err = esp_vfs_fat_sdmmc_mount("/sd", &host, &slot_config, &mount_config, &card);
//             if(err == ESP_OK) 
//                 break;
//         }
//     SDL_UnlockDisplay();

//     printf("Init_SD: SD card opened.\n");
    
// 	//sdmmc_card_print_info(stdout, card);    
// }

#include <stdio.h>
#include <string.h>
#include <sys/dirent.h>
#include <sys/unistd.h>
#include <sys/stat.h>

// Function to list files in a directory
void listFiles(const char *dirname) {
    DIR *dir;
    struct dirent *entry;

    // Open the directory
    dir = opendir(dirname);
    if (!dir) {
        printf("Failed to open directory: %s\n", dirname);
        return;
    }

    // Read directory entries
    while ((entry = readdir(dir)) != NULL) {
        struct stat entry_stat;
        char path[1024];

        // Build full path for stat
        snprintf(path, sizeof(path), "%s/%s", dirname, entry->d_name);

        // Get entry status
        if (stat(path, &entry_stat) == -1) {
            printf("Failed to stat %s\n", path);
            continue;
        }

        // Check if it's a directory or a file
        if (S_ISDIR(entry_stat.st_mode)) {
            printf("[DIR]  %s\n", entry->d_name);
        } else if (S_ISREG(entry_stat.st_mode)) {
            printf("[FILE] %s (Size: %ld bytes)\n", entry->d_name, entry_stat.st_size);
        }
    }

    // Close the directory
    closedir(dir);
}

void SDL_InitFS(void) {
    printf("Initialising File System\n");

    // Define the LittleFS configuration
    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/sd",
        .partition_label = "storage",
        .format_if_mount_failed = false,
        .dont_mount = false,
    };

    // Use the API to mount and possibly format the file system
    esp_err_t err = esp_vfs_littlefs_register(&conf);
    if (err != ESP_OK) {
        printf("Failed to mount or format filesystem\n");
    } else {
        printf("Filesystem mounted\n");
        printf("Listing files in /:\n");
        listFiles("/sd");
    }
}

const SDL_version* SDL_Linked_Version()
{
    SDL_version *vers = malloc(sizeof(SDL_version));
    vers->major = SDL_MAJOR_VERSION;                 
    vers->minor = SDL_MINOR_VERSION;                 
    vers->patch = SDL_PATCHLEVEL;      
    return vers;
}

char *** allocateTwoDimenArrayOnHeapUsingMalloc(int row, int col)
{
	char ***ptr = malloc(row * sizeof(*ptr) + row * (col * sizeof **ptr) );

	int * const data = ptr + row;
	for(int i = 0; i < row; i++)
		ptr[i] = data + i * col;

	return ptr;
}

void SDL_DestroyMutex(SDL_mutex* mutex)
{

}

SDL_mutex* SDL_CreateMutex(void)
{
    SDL_mutex* mut = NULL;
    return mut;
}

int SDL_LockMutex(SDL_mutex* mutex)
{
    return 0;
}

int SDL_UnlockMutex(SDL_mutex* mutex)
{
    return 0;
}