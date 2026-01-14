/* 
 * OpenTyrian: A modern cross-platform port of Tyrian
 * Copyright (C) 2007-2009  The OpenTyrian Development Team
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
#include "file.h"
#include "opentyr.h"
#include "varz.h"

#include "SDL3/SDL.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "esp_vfs_fat.h"
#include "esp_vfs.h"
#include "esp_littlefs.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"

// Include BSP SD card support for ESP32-P4 Function EV Board
#if CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV
#include "bsp/esp32_p4_function_ev_board.h"
#endif

#define MODE_SPI 1
#define PIN_NUM_MISO CONFIG_HW_SD_PIN_NUM_MISO
#define PIN_NUM_MOSI CONFIG_HW_SD_PIN_NUM_MOSI
#define PIN_NUM_CLK  CONFIG_HW_SD_PIN_NUM_CLK
#define PIN_NUM_CS   CONFIG_HW_SD_PIN_NUM_CS

// Track which filesystem is available
static bool sd_card_available = false;
static bool flash_available = false;

const char *custom_data_dir = NULL;

static bool init_SD = false;


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



// Check if Tyrian data exists on SD card
static bool check_sd_card_data(void) {
    const char *test_file = BSP_SD_MOUNT_POINT "/tyrian/data/tyrian1.lvl";
    FILE *f = fopen(test_file, "r");

    if (f) {
        fclose(f);
        printf("Found Tyrian data on SD card at %s\n", BSP_SD_MOUNT_POINT "/tyrian/data");
        return true;
    }

    printf("Tyrian data not found on SD card (checked %s)\n", test_file);
    return false;
}

// Check if Tyrian data exists on internal flash
static bool check_flash_data(void) {
    const char *test_file = "/flash/tyrian/data/tyrian1.lvl";
    FILE *f = fopen(test_file, "r");

    if (f) {
        fclose(f);
        printf("Found Tyrian data on internal flash at /flash/tyrian/data\n");
        return true;
    }

    printf("Tyrian data not found on internal flash (checked %s)\n", test_file);
    return false;
}

void SDL_InitFS(void) {
    printf("==============================================\n");
    printf("OpenTyrian File System Initialization\n");
    printf("==============================================\n");

#if CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV
    // Try to mount SD card first (if ESP32-P4 Function EV Board)
    printf("\n[1/2] Attempting to mount SD card...\n");

    esp_err_t sd_err = bsp_sdcard_mount();
    if (sd_err == ESP_OK) {
        printf("SUCCESS: SD card mounted at %s\n", BSP_SD_MOUNT_POINT);

        // Check if Tyrian data exists on SD card
        if (check_sd_card_data()) {
            sd_card_available = true;
            printf("\n>>> Using SD card for game data <<<\n");
            init_SD = true;
            return;
        } else {
            printf("INFO: SD card mounted but no Tyrian data found\n");
            // Unmount SD card since we don't need it
            bsp_sdcard_unmount();
        }
    } else {
        printf("INFO: SD card not available or mount failed (error: 0x%x)\n", sd_err);
        printf("      This is normal if no SD card is inserted\n");
    }
#else
    printf("INFO: SD card support not available for this board\n");
#endif

    // Fall back to internal flash LittleFS
    printf("\n[2/2] Attempting to mount internal flash LittleFS...\n");

    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/flash",
        .partition_label = "storage",
        .format_if_mount_failed = false,
        .dont_mount = false,
    };

    esp_err_t flash_err = esp_vfs_littlefs_register(&conf);
    if (flash_err != ESP_OK) {
        printf("ERROR: Failed to mount internal flash filesystem (error: 0x%x)\n", flash_err);
        printf("FATAL ERROR: No game data source available!\n");
        printf("Please ensure either:\n");
        printf("  1. SD card with tyrian/data is inserted, OR\n");
        printf("  2. Firmware includes LittleFS partition with game data\n");
        return;
    }

    printf("SUCCESS: Internal flash LittleFS mounted at /flash\n");

    // Verify data exists on flash
    if (check_flash_data()) {
        flash_available = true;
        printf("\n>>> Using internal flash for game data <<<\n");
        printf("Listing files in /flash:\n");
        listFiles("/flash");
    } else {
        printf("ERROR: No Tyrian data found on internal flash!\n");
        printf("Please flash firmware with embedded game data or use SD card\n");
    }

    printf("==============================================\n");
    init_SD = true;
}

void Init_SD()
{
	SDL_InitFS();
	//sdmmc_card_print_info(stdout, card);
	init_SD = true;
}

// finds the Tyrian data directory
const char *data_dir( void )
{
	// Return the appropriate path based on which filesystem is available
#if CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV
	if (sd_card_available) {
		printf("data_dir: Using SD card path: %s/tyrian/data\n", BSP_SD_MOUNT_POINT);
		return BSP_SD_MOUNT_POINT "/tyrian/data";
	}
#endif

	if (flash_available) {
		printf("data_dir: Using flash path: /flash/tyrian/data\n");
		return "/flash/tyrian/data";
	}

	// Fallback to original logic for custom data dir or other configurations
	const char *dirs[] =
	{
#if CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV
		BSP_SD_MOUNT_POINT "/tyrian/data",  // Try SD card first
#endif
		"/flash/tyrian/data",               // Then try flash
		custom_data_dir,                    // Custom path if set
		TYRIAN_DIR,                         // Default path
		"data",                             // Relative path
		".",                                // Current directory
	};

	static const char *dir = NULL;

	if (dir != NULL)
		return dir;

	for (uint i = 0; i < COUNTOF(dirs); ++i)
	{
		if (dirs[i] == NULL)
			continue;

		FILE *f = dir_fopen(dirs[i], "tyrian1.lvl", "rb");
		if (f)
		{
			efclose(f);

			dir = dirs[i];
			printf("data_dir: Found Tyrian data at: %s\n", dir);
			break;
		}
	}

	if (dir == NULL) // data not found
		dir = "";

	return dir;
}

// prepend directory and fopen
FILE *dir_fopen( const char *dir, const char *file, const char *mode )
{
	char path[512]; 
	fprintf(stderr, "Opening File: %s/%s\n", dir, file);
	if(init_SD == false)
		Init_SD();

	//char *path = (char *)malloc(strlen(dir) + 1 + strlen(file) + 1);
	sprintf(path, "%s/%s", dir, file);
	
	// SDL_LockDisplay();
	FILE *f = fopen(path, mode);
	// SDL_UnlockDisplay();
	
	//free(path);
	
	return f;
}

// warn when dir_fopen fails
FILE *dir_fopen_warn(  const char *dir, const char *file, const char *mode )
{
	FILE *f = dir_fopen(dir, file, mode);
	
	if (f == NULL)
		fprintf(stderr, "warning: failed to open '%s': %s\n", file, strerror(errno));
	
	return f;
}

// die when dir_fopen fails
FILE *dir_fopen_die( const char *dir, const char *file, const char *mode )
{
	FILE *f = dir_fopen(dir, file, mode);
	
	if (f == NULL)
	{
		fprintf(stderr, "error: failed to open '%s': %s\n", file, strerror(errno));
		fprintf(stderr, "error: One or more of the required Tyrian " TYRIAN_VERSION " data files could not be found.\n"
		                "       Please read the README file.\n");
		JE_tyrianHalt(1);
	}
	
	return f;
}

// check if file can be opened for reading
bool dir_file_exists( const char *dir, const char *file )
{
	FILE *f = dir_fopen(dir, file, "rb");
	if (f != NULL)
	{
		efclose(f);
	}
	return (f != NULL);
}

// returns end-of-file position
long ftell_eof( FILE *f )
{
	// SDL_LockDisplay();
	long pos = ftell(f);
	
	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	
	fseek(f, pos, SEEK_SET);
	// SDL_UnlockDisplay();
	return size;
}

int efeof ( FILE * stream )
{
	// SDL_LockDisplay();
	int ret = feof ( stream );
	// SDL_UnlockDisplay();
	return ret;	
}

int efputc ( int character, FILE * stream )
{
	// SDL_LockDisplay();
	int ret = fputc ( character, stream );
	// SDL_UnlockDisplay();
	return ret;	
}

int efgetc ( FILE * stream )
{
	// SDL_LockDisplay();
	int ret = fgetc ( stream );
	// SDL_UnlockDisplay();
	return ret;	
}

size_t eefwrite ( const void * ptr, size_t size, size_t count, FILE * stream )
{
	// SDL_LockDisplay();
	size_t ret = fwrite ( ptr, size, count, stream );
	// SDL_UnlockDisplay();
	return ret;		
}

int efclose ( FILE * stream )
{
	// SDL_LockDisplay();
	int ret = fclose ( stream );
	// SDL_UnlockDisplay();
	return ret;	
}

long int eftell ( FILE * stream )
{
	// SDL_LockDisplay();
	long int ret = ftell ( stream );
	// SDL_UnlockDisplay();
	return ret;
}

int efseek( FILE * stream, long int offset, int origin )
{
	// SDL_LockDisplay();
	int ret = fseek ( stream, offset, origin );
	// SDL_UnlockDisplay();
	return ret;
}

// endian-swapping fread that dies if the expected amount cannot be read
size_t efread( void *buffer, size_t size, size_t num, FILE *stream )
{
	// SDL_LockDisplay();
	size_t num_read = fread(buffer, size, num, stream);
	// SDL_UnlockDisplay();

	switch (size)
	{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		case 2:
			for (size_t i = 0; i < num; i++)
				((Uint16 *)buffer)[i] = SDL_Swap16(((Uint16 *)buffer)[i]);
			break;
		case 4:
			for (size_t i = 0; i < num; i++)
				((Uint32 *)buffer)[i] = SDL_Swap32(((Uint32 *)buffer)[i]);
			break;
		case 8:
			for (size_t i = 0; i < num; i++)
				((Uint64 *)buffer)[i] = SDL_Swap64(((Uint64 *)buffer)[i]);
			break;
#endif
		default:
			break;
	}
	
	if (num_read != num)
	{
		fprintf(stderr, "error: An unexpected problem occurred while reading from a file.\n");
		fprintf(stderr, "read bytes: %d, expected: %d\n", num_read, num);
		JE_tyrianHalt(1);
	}

	return num_read;
}

// endian-swapping fwrite that dies if the expected amount cannot be written
size_t efwrite( const void *buffer, size_t size, size_t num, FILE *stream )
{
	void *swap_buffer = NULL;
	
	switch (size)
	{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		case 2:
			swap_buffer = malloc(size * num);
			for (size_t i = 0; i < num; i++)
				((Uint16 *)swap_buffer)[i] = SDL_Swap16LE(((Uint16 *)buffer)[i]);
			buffer = swap_buffer;
			break;
		case 4:
			swap_buffer = malloc(size * num);
			for (size_t i = 0; i < num; i++)
				((Uint32 *)swap_buffer)[i] = SDL_Swap32LE(((Uint32 *)buffer)[i]);
			buffer = swap_buffer;
			break;
		case 8:
			swap_buffer = malloc(size * num);
			for (size_t i = 0; i < num; i++)
				((Uint64 *)swap_buffer)[i] = SDL_SwapLE64(((Uint64 *)buffer)[i]);
			buffer = swap_buffer;
			break;
#endif
		default:
			break;
	}
	
	// SDL_LockDisplay();
	size_t num_written = fwrite(buffer, size, num, stream);
	// SDL_UnlockDisplay();

	if (swap_buffer != NULL)
		free(swap_buffer);
	
	if (num_written != num)
	{
		fprintf(stderr, "error: An unexpected problem occurred while writing to a file.\n");
		JE_tyrianHalt(1);
	}
	
	return num_written;
}
