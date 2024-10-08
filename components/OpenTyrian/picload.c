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
#include "palette.h"
#include "pcxmast.h"
#include "picload.h"
#include "video.h"
#include "esp_heap_caps.h"


#include <string.h>

void check_memory() {
    size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    size_t free_dram = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

    printf("Available PSRAM: %zu bytes\n", free_psram);
    printf("Available DRAM: %zu bytes\n", free_dram);
}

void JE_loadPic(SDL_Surface *screen, JE_byte PCXnumber, JE_boolean storepal )
{
	PCXnumber--;

	FILE *f = dir_fopen_die(data_dir(), "tyrian.pic", "rb");

	static bool first = true;
	if (first)
	{
		first = false;

		Uint16 temp;
		efread(&temp, sizeof(Uint16), 1, f);
		for (int i = 0; i < PCX_NUM; i++)
		{
			efread(&pcxpos[i], sizeof(JE_longint), 1, f);
		}

		pcxpos[PCX_NUM] = ftell_eof(f);
	}

	unsigned int size = pcxpos[PCXnumber + 1] - pcxpos[PCXnumber];
	printf("size: %d\n", size);

	check_memory();

	Uint8 *buffer = (Uint8 *)heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
	if (buffer == NULL) {
		printf("Unable to allocate memory in PSRAM for reading file: %i\n", size);
		return;
	} else {
		printf("Allocated: %i\n", size);
	}
	// Uint8 *buffer = (Uint8 *)malloc(size);
	// if (buffer == NULL) {
	// 	printf("Unable to allocate memory for reading file: %i\n", size);
	// 	return;
	// }
	efseek(f, pcxpos[PCXnumber], SEEK_SET);
	efread(buffer, sizeof(Uint8), size, f);
	efclose(f);

	Uint8 *p = buffer;
	Uint8 *s; /* screen pointer, 8-bit specific */

	s = (Uint8 *)screen->pixels;

	for (int i = 0; i < 320 * 200; )
	{
		if ((*p & 0xc0) == 0xc0)
		{
			i += (*p & 0x3f);
			memset(s, *(p + 1), (*p & 0x3f));
			s += (*p & 0x3f); p += 2;
		} else {
			i++;
			*s = *p;
			s++; p++;
		}
		if (i && (i % 320 == 0))
		{
			s += screen->pitch - 320;
		}
	}

	free(buffer);

	memcpy(colors, palettes[pcxpal[PCXnumber]], sizeof(colors));

	if (storepal)
		set_palette(colors, 0, 255);
}

