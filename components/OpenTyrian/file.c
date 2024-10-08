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

// #include "esp_vfs_fat.h"
#include "esp_vfs.h"
#include "esp_littlefs.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"

#define MODE_SPI 1
#define PIN_NUM_MISO CONFIG_HW_SD_PIN_NUM_MISO
#define PIN_NUM_MOSI CONFIG_HW_SD_PIN_NUM_MOSI
#define PIN_NUM_CLK  CONFIG_HW_SD_PIN_NUM_CLK
#define PIN_NUM_CS   CONFIG_HW_SD_PIN_NUM_CS

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

void Init_SD()
{
	SDL_InitFS();
	//sdmmc_card_print_info(stdout, card);
	init_SD = true;
}

// finds the Tyrian data directory
const char *data_dir( void )
{
	return "/sd/tyrian/data";
	const char *dirs[] =
	{
		"/sd/tyrian/data",
		custom_data_dir,
		TYRIAN_DIR,
		"data",
		".",
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
