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
#include "keyboard.h"
#include "opentyr.h"
#include "palette.h"
#include "video.h"
#include "video_scale.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

bool fullscreen_enabled = false;

SDL_Surface *VGAScreen, *VGAScreenSeg;
SDL_Surface *VGAScreen2;
SDL_Surface *game_screen;

static ScalerFunction scaler_function;

void init_video( void )
{
	if (SDL_WasInit(SDL_INIT_VIDEO))
		return;

	if (SDL_InitSubSystem(SDL_INIT_VIDEO) == -1)
	{
		fprintf(stderr, "error: failed to initialize SDL video: %s\n", SDL_GetError());
		exit(1);
	}

	// SDL_WM_SetCaption("OpenTyrian", NULL);
//heap_caps_check_integrity_all(true);
	VGAScreen = VGAScreenSeg = SDL_CreateSurface(vga_width, vga_height, SDL_PIXELFORMAT_INDEX8);
	VGAScreen2 = SDL_CreateSurface(vga_width, vga_height, SDL_PIXELFORMAT_INDEX8);
	game_screen = SDL_CreateSurface(vga_width, vga_height, SDL_PIXELFORMAT_INDEX8);

// printf("BPP: %d\n", VGAScreen->format->BitsPerPixel);
	// spi_lcd_clear();
	SDL_FillSurfaceRect(VGAScreen, NULL, 0);
//heap_caps_check_integrity_all(true);	
/*
	if (!init_scaler(scaler, fullscreen_enabled) &&  // try desired scaler and desired fullscreen state
	    !init_any_scaler(fullscreen_enabled) &&      // try any scaler in desired fullscreen state
	    !init_any_scaler(!fullscreen_enabled))       // try any scaler in other fullscreen state
	{
		fprintf(stderr, "error: failed to initialize any supported video mode\n");
		exit(EXIT_FAILURE);
	}
	*/
}

int can_init_scaler( unsigned int new_scaler, bool fullscreen )
{
	if (new_scaler >= scalers_count)
		return false;
	
	int w = scalers[new_scaler].width,
	    h = scalers[new_scaler].height;
	// int flags = SDL_SWSURFACE | SDL_HWPALETTE | (fullscreen ? SDL_FULLSCREEN : 0);

	int flags = 0;	
	// test each bitdepth
	for (uint bpp = 32; bpp > 0; bpp -= 8)
	{
		// uint temp_bpp = SDL_VideoModeOK(w, h, bpp, flags);
		uint temp_bpp = 8;
		
		if ((temp_bpp == 32 && scalers[new_scaler].scaler32) ||
		    (temp_bpp == 16 && scalers[new_scaler].scaler16) ||
		    (temp_bpp == 8  && scalers[new_scaler].scaler8 ))
		{
			return temp_bpp;
		}
		else if (temp_bpp == 24 && scalers[new_scaler].scaler32)
		{
			// scalers don't support 24 bpp because it's a pain
			// so let SDL handle the conversion
			return 32;
		}
	}
	
	return 0;
}

SDL_Window *window;
SDL_Renderer *renderer;
bool init_scaler( unsigned int new_scaler, bool fullscreen )
{
	int w = scalers[new_scaler].width,
	    h = scalers[new_scaler].height;
	int bpp = can_init_scaler(new_scaler, fullscreen);
	// int flags = SDL_SWSURFACE | SDL_HWPALETTE | (fullscreen ? SDL_FULLSCREEN : 0);
	int flags = 0;
	
	if (bpp == 0)
		return false;
	
	// SDL_Surface *const surface = SDL_SetVideoMode(w, h, bpp, flags);

    window = SDL_CreateWindow("SDL on ESP32", 320, 240, 0);
    if (!window) {
        printf("Failed to create window: %s\n", SDL_GetError());
        return;
    }

    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        printf("Failed to create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        return;
    }
	
	// if (surface == NULL)
	// {
	// 	fprintf(stderr, "error: failed to initialize %s video mode %dx%dx%d: %s\n", fullscreen ? "fullscreen" : "windowed", w, h, bpp, SDL_GetError());
	// 	return false;
	// }
	
	// w = surface->w;
	// h = surface->h;
	// // bpp = surface->BitsPerPixel;
	// bpp = 8;
	
	// printf("initialized video: %dx%dx%d %s\n", w, h, bpp, fullscreen ? "fullscreen" : "windowed");
	
	scaler = new_scaler;
	fullscreen_enabled = fullscreen;
	
	switch (bpp)
	{
	case 32:
		scaler_function = scalers[scaler].scaler32;
		break;
	case 16:
		scaler_function = scalers[scaler].scaler16;
		break;
	case 8:
		scaler_function = scalers[scaler].scaler8;
		break;
	default:
		scaler_function = NULL;
		break;
	}
	
	if (scaler_function == NULL)
	{
		assert(false);
		return false;
	}
	
	input_grab(input_grab_enabled);
	
	JE_showVGA();
	
	return true;
}

bool can_init_any_scaler( bool fullscreen )
{
	for (int i = scalers_count - 1; i >= 0; --i)
		if (can_init_scaler(i, fullscreen) != 0)
			return true;
	
	return false;
}

bool init_any_scaler( bool fullscreen )
{
	// attempts all scalers from last to first
	for (int i = scalers_count - 1; i >= 0; --i)
		if (init_scaler(i, fullscreen))
			return true;
	
	return false;
}

void deinit_video( void )
{
	SDL_DestroySurface(VGAScreenSeg);
	SDL_DestroySurface(VGAScreen2);
	SDL_DestroySurface(game_screen);
	
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void JE_clr256( SDL_Surface * screen)
{
	memset(screen->pixels, 0, screen->pitch * screen->h);
}
void JE_showVGA( void ) { scale_and_flip(VGAScreen); }


void scale_and_flip(SDL_Surface *src_surface)
{
    // Convert the SDL_Surface to an SDL_Texture
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, src_surface);
    if (!texture) {
        SDL_Log("Failed to create texture: %s", SDL_GetError());
        return;
    }

    // Clear the renderer
    SDL_RenderClear(renderer);

    // Get the window dimensions
    int window_width, window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);

    // Define the destination rectangle for scaling
    SDL_FRect dst_rect = { 0, 0, window_width, window_height };

    // Copy the texture to the renderer, scaling it to fit the window
    SDL_RenderTexture(renderer, texture, NULL, &dst_rect);

    // Present the renderer (equivalent to SDL_Flip in SDL2)
    SDL_RenderPresent(renderer);

    // Cleanup: destroy the texture after rendering
    SDL_DestroyTexture(texture);
}
