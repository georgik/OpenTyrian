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
#include "nortsong.h"
#include "opentyr.h"
#include "palette.h"
#include "video.h"

#include <assert.h>

static Uint32 rgb_to_yuv( int r, int g, int b );

#ifdef TYRIAN2000
#define PALETTE_COUNT 24
#else
#define PALETTE_COUNT 23
#endif

EXT_RAM_BSS_ATTR Palette palettes[PALETTE_COUNT];
int palette_count;

// EXT_RAM_BSS_ATTR static Palette palette;
SDL_Palette *palette;

EXT_RAM_BSS_ATTR Uint32 rgb_palette[256], yuv_palette[256];

EXT_RAM_BSS_ATTR Palette colors;

void JE_loadPals( void )
{
	FILE *f = dir_fopen_die(data_dir(), "palette.dat", "rb");
	
	palette_count = ftell_eof(f) / (256 * 3);
	assert(palette_count == PALETTE_COUNT);
	//palettes = (Palette *)malloc(sizeof(Palette) * PALETTE_COUNT);
	for (int p = 0; p < palette_count; ++p)
	{
		for (int i = 0; i < 256; ++i)
		{
			// The VGA hardware palette used only 6 bits per component, so the values need to be rescaled to
			// 8 bits. The naive way to do this is to simply do (c << 2), padding it with 0's, however this
			// makes the maximum value 252 instead of the proper 255. A trick to fix this is to use the upper 2
			// bits of the original value instead. This ensures that the value goes to 255 as the original goes
			// to 63.

			int c = efgetc(f);
			palettes[p][i].r = (c << 2) | (c >> 4);
			c = efgetc(f);
			palettes[p][i].g = (c << 2) | (c >> 4);
			c = efgetc(f);
			palettes[p][i].b = (c << 2) | (c >> 4);
		}
	}
	
	efclose(f);
}

void set_palette(Palette colors, unsigned int first_color, unsigned int last_color) {
    for (uint i = first_color; i <= last_color; ++i) {
        palette->colors[i] = colors[i];  // Corrected to use palette->colors
    }

    // Update the colors in the palette
    SDL_SetPaletteColors(palette, colors, first_color, last_color - first_color + 1);

    // Make sure to apply the palette to all relevant surfaces
    SDL_SetSurfacePalette(VGAScreen, palette);
    SDL_SetSurfacePalette(VGAScreen2, palette);
    SDL_SetSurfacePalette(game_screen, palette);
}

void set_colors(SDL_Color color, unsigned int first_color, unsigned int last_color) {
    for (uint i = first_color; i <= last_color; ++i) {
        palette->colors[i] = color;  // Corrected to use palette->colors
    }

    // Update the colors in the palette
    SDL_SetPaletteColors(palette, &palette->colors[first_color], first_color, last_color - first_color + 1);
}


void init_step_fade_palette(int diff[256][3], Palette colors, unsigned int first_color, unsigned int last_color) {
    for (unsigned int i = first_color; i <= last_color; i++) {
        diff[i][0] = (int)colors[i].r - palette->colors[i].r;
        diff[i][1] = (int)colors[i].g - palette->colors[i].g;
        diff[i][2] = (int)colors[i].b - palette->colors[i].b;
    }
}


void init_step_fade_solid(int diff[256][3], SDL_Color color, unsigned int first_color, unsigned int last_color) {
    for (unsigned int i = first_color; i <= last_color; i++) {
        diff[i][0] = (int)color.r - palette->colors[i].r;
        diff[i][1] = (int)color.g - palette->colors[i].g;
        diff[i][2] = (int)color.b - palette->colors[i].b;
    }
}


void step_fade_palette(int diff[256][3], int steps, unsigned int first_color, unsigned int last_color) {
    assert(steps > 0);
    
    for (unsigned int i = first_color; i <= last_color; i++) {
        int delta[3] = { diff[i][0] / steps, diff[i][1] / steps, diff[i][2] / steps };
        
        diff[i][0] -= delta[0];
        diff[i][1] -= delta[1];
        diff[i][2] -= delta[2];
        
        palette->colors[i].r += delta[0];
        palette->colors[i].g += delta[1];
        palette->colors[i].b += delta[2];
    }

    SDL_SetPaletteColors(palette, &palette->colors[first_color], first_color, last_color - first_color + 1);
}



void fade_palette(Palette colors, int steps, unsigned int first_color, unsigned int last_color)
{
    assert(steps > 0);
    
    static int diff[256][3];
    init_step_fade_palette(diff, colors, first_color, last_color);
    
    for (; steps > 0; steps--)
    {
        setdelay(1);
        
        step_fade_palette(diff, steps, first_color, last_color);
        
        // No need to handle non-8bpp logic
        wait_delay();
    }
}


void fade_solid(SDL_Color color, int steps, unsigned int first_color, unsigned int last_color)
{
    assert(steps > 0);
    
    static int diff[256][3];
    init_step_fade_solid(diff, color, first_color, last_color);
    
    for (; steps > 0; steps--)
    {
        setdelay(1);
        
        step_fade_palette(diff, steps, first_color, last_color);
        
        // No need to handle non-8bpp logic
        wait_delay();
    }
}


void fade_black( int steps )
{
	SDL_Color black = { 0, 0, 0 };
	fade_solid(black, steps, 0, 255);
}

void fade_white( int steps )
{
	SDL_Color white = { 255, 255, 255 };
	fade_solid(white, steps, 0, 255);
}

static Uint32 rgb_to_yuv( int r, int g, int b )
{
	int y = (r + g + b) >> 2,
	    u = 128 + ((r - b) >> 2),
	    v = 128 + ((-r + 2 * g - b) >> 3);
	return (y << 16) + (u << 8) + v;
}

