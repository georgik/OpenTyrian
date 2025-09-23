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

// Function declaration for scaling setup (defined in app_main.c)
extern void setup_sdl_scaling(SDL_Renderer *renderer);

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <time.h>  // Include for time functions

// Define variables for FPS calculation
static uint32_t frame_count = 0;
static uint32_t start_time = 0;

bool fullscreen_enabled = false;

SDL_Surface *VGAScreen, *VGAScreenSeg;
SDL_Surface *VGAScreen2;
SDL_Surface *game_screen;

SDL_Window *window = NULL;
SDL_Renderer *renderer = NULL;

static ScalerFunction scaler_function;

// int scale_factor = 1;

void clear_screen(SDL_Renderer *renderer)
{
    SDL_SetRenderDrawColor(renderer, 88, 66, 255, 255);
    SDL_RenderClear(renderer);
}

void init_video(void)
{
    if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) == false) {
        printf("Unable to initialize SDL: %s\n", SDL_GetError());
        return;
    }
    printf("SDL initialized successfully\n");

#ifdef CONFIG_SDL_BSP_M5STACK_TAB5
    // M5Stack Tab5: 1280x720 display - but we need to account for rotation
#    ifdef CONFIG_OPENTYRIAN_ROTATION_90
    // 90-degree rotation: swap width/height for window creation
    window = SDL_CreateWindow("OpenTyrian on M5Stack Tab5", 720, 1280, 0);
#    else
    window = SDL_CreateWindow("OpenTyrian on M5Stack Tab5", 1280, 720, 0);
#    endif
#elif defined(CONFIG_SDL_BSP_ESP32_P4_FUNCTION_EV)
    // ESP32-P4 Function EV Board: 1024x600 display
    window = SDL_CreateWindow("OpenTyrian on ESP32-P4", 1024, 600, 0);
#elif defined(CONFIG_SDL_BSP_ESP_BOX_3)
    // ESP-Box-3: 320x240 display
    window = SDL_CreateWindow("OpenTyrian on ESP-Box-3", 320, 240, 0);
#else
    // Default/fallback resolution
    window = SDL_CreateWindow("OpenTyrian", 320, 200, 0);
#endif
    if(!window) {
        printf("Failed to create window: %s\n", SDL_GetError());
        return;
    }

    // Set up rotation AND scale factor BEFORE creating renderer/framebuffer (ESP-IDF needs this)
#ifdef CONFIG_IDF_TARGET_ESP32P4
    // Function declarations (defined in SDL ESP-IDF framebuffer)
    extern void set_display_rotation(int angle);
    extern void set_display_scale_factor(int factor);
    int rotation_angle = CONFIG_OPENTYRIAN_ROTATION_ANGLE;
    set_display_rotation(rotation_angle);
    printf("Display rotation set to %d degrees (before framebuffer creation)\n", rotation_angle);

    // Set scale factor before framebuffer creation so PPA buffer is allocated correctly
    int scale_factor = CONFIG_OPENTYRIAN_SCALE_FACTOR_INT;
    set_display_scale_factor(scale_factor);
    printf("Display scale factor set to %dx (before framebuffer creation)\n", scale_factor);
#endif

    // For ESP-IDF, try to create a renderer but continue even if it fails
    // ESP-IDF uses framebuffer rendering, not traditional render drivers
    renderer = SDL_CreateRenderer(window, NULL);
    if(!renderer) {
        // List available render drivers for debugging
        int num_drivers = SDL_GetNumRenderDrivers();
        printf("Failed to create renderer: %s\n", SDL_GetError());
        printf("Available render drivers (%d):\n", num_drivers);
        for(int i = 0; i < num_drivers; i++) {
            const char *driver_name = SDL_GetRenderDriver(i);
            if(driver_name) {
                printf("  %d: %s\n", i, driver_name);
            }
        }
        printf("Continuing without renderer (ESP-IDF uses framebuffer rendering)\n");
    }

    // Apply scaling configuration from Kconfig (even if renderer is NULL)
    setup_sdl_scaling(renderer);

    // Apply scaling configuration from Kconfig
    setup_sdl_scaling(renderer);

    if(renderer) {
        clear_screen(renderer);
        SDL_RenderPresent(renderer);
    } else {
        printf("No renderer, skipping clear/present operations\n");
    }


    // SDL_WM_SetCaption("OpenTyrian", NULL);
    // heap_caps_check_integrity_all(true);
    //  VGAScreen = VGAScreenSeg = SDL_CreateSurface(vga_width, vga_height, SDL_PIXELFORMAT_RGB565);
    //  VGAScreen2 = SDL_CreateSurface(vga_width, vga_height, SDL_PIXELFORMAT_RGB565);
    //  game_screen = SDL_CreateSurface(vga_width, vga_height, SDL_PIXELFORMAT_RGB565);

    VGAScreen = VGAScreenSeg = SDL_CreateSurface(vga_width, vga_height, SDL_PIXELFORMAT_INDEX8);
    VGAScreen2 = SDL_CreateSurface(vga_width, vga_height, SDL_PIXELFORMAT_INDEX8);
    game_screen = SDL_CreateSurface(vga_width, vga_height, SDL_PIXELFORMAT_INDEX8);

    // Debug: Log the game surface sizes
    printf("DEBUG: Created VGAScreen: %dx%d, VGAScreen2: %dx%d, game_screen: %dx%d\n",
           VGAScreen->w,
           VGAScreen->h,
           VGAScreen2->w,
           VGAScreen2->h,
           game_screen->w,
           game_screen->h);

    palette = SDL_CreatePalette(256);

    SDL_SetSurfacePalette(VGAScreen, palette);   // Set the palette for VGAScreen
    SDL_SetSurfacePalette(VGAScreen2, palette);  // Set the same palette for VGAScreen2
    SDL_SetSurfacePalette(game_screen, palette);

    // printf("BPP: %d\n", VGAScreen->format->BitsPerPixel);
    // spi_lcd_clear();
    SDL_FillSurfaceRect(VGAScreen, NULL, 0);
    // heap_caps_check_integrity_all(true);

#ifdef CONFIG_IDF_TARGET_ESP32P4
    // Force initialization of "None" scaler (index 0) for ESP32-P4
    if(!init_scaler(0, fullscreen_enabled)) {
        fprintf(stderr, "error: failed to initialize None scaler for ESP32-P4\n");
        exit(EXIT_FAILURE);
    }
    printf("ESP32-P4: Initialized with 'None' scaler for native resolution rendering\n");
#else
    // Original desktop/other target scaler initialization
    if(!init_scaler(scaler, fullscreen_enabled) &&  // try desired scaler and desired fullscreen state
       !init_any_scaler(fullscreen_enabled) &&      // try any scaler in desired fullscreen state
       !init_any_scaler(!fullscreen_enabled))       // try any scaler in other fullscreen state
    {
        fprintf(stderr, "error: failed to initialize any supported video mode\n");
        exit(EXIT_FAILURE);
    }
#endif
}

int can_init_scaler(unsigned int new_scaler, bool fullscreen)
{
    if(new_scaler >= scalers_count)
        return false;

    int w = scalers[new_scaler].width, h = scalers[new_scaler].height;
    // int flags = SDL_SWSURFACE | SDL_HWPALETTE | (fullscreen ? SDL_FULLSCREEN : 0);

    int flags = 0;
    // test each bitdepth
    for(uint bpp = 32; bpp > 0; bpp -= 8) {
        // uint temp_bpp = SDL_VideoModeOK(w, h, bpp, flags);
        uint temp_bpp = 8;

        if((temp_bpp == 32 && scalers[new_scaler].scaler32) || (temp_bpp == 16 && scalers[new_scaler].scaler16) ||
           (temp_bpp == 8 && scalers[new_scaler].scaler8)) {
            return temp_bpp;
        } else if(temp_bpp == 24 && scalers[new_scaler].scaler32) {
            // scalers don't support 24 bpp because it's a pain
            // so let SDL handle the conversion
            return 32;
        }
    }

    return 0;
}


bool init_scaler(unsigned int new_scaler, bool fullscreen)
{
    int w = scalers[new_scaler].width, h = scalers[new_scaler].height;
    // int bpp = can_init_scaler(new_scaler, fullscreen);
    int bpp = 8;
    // int flags = SDL_SWSURFACE | SDL_HWPALETTE | (fullscreen ? SDL_FULLSCREEN : 0);
    int flags = 0;

    if(bpp == 0)
        return false;

    // SDL_Surface *const surface = SDL_SetVideoMode(w, h, bpp, flags);


    // if (surface == NULL)
    // {
    // 	fprintf(stderr, "error: failed to initialize %s video mode %dx%dx%d: %s\n", fullscreen ? "fullscreen" :
    // "windowed", w, h, bpp, SDL_GetError()); 	return false;
    // }

    // w = surface->w;
    // h = surface->h;
    // // bpp = surface->BitsPerPixel;
    // bpp = 8;

    // printf("initialized video: %dx%dx%d %s\n", w, h, bpp, fullscreen ? "fullscreen" : "windowed");

    scaler = new_scaler;
    fullscreen_enabled = fullscreen;

    switch(bpp) {
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

    if(scaler_function == NULL) {
        assert(false);
        return false;
    }

    input_grab(input_grab_enabled);

    JE_showVGA();

    return true;
}

bool can_init_any_scaler(bool fullscreen)
{
    for(int i = scalers_count - 1; i >= 0; --i)
        if(can_init_scaler(i, fullscreen) != 0)
            return true;

    return false;
}

bool init_any_scaler(bool fullscreen)
{
    // attempts all scalers from last to first
    for(int i = scalers_count - 1; i >= 0; --i)
        if(init_scaler(i, fullscreen))
            return true;

    return false;
}

void deinit_video(void)
{
    SDL_DestroySurface(VGAScreenSeg);
    SDL_DestroySurface(VGAScreen2);
    SDL_DestroySurface(game_screen);

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void JE_clr256(SDL_Surface *screen)
{
    memset(screen->pixels, 0, screen->pitch * screen->h);
}
void JE_showVGA(void)
{
#ifdef CONFIG_IDF_TARGET_ESP32P4
    // ESP32-P4: Direct blitting without old scaler system
    scale_and_flip(VGAScreen);
#else
    // Other targets: Use old scaler system
    scale_and_flip(VGAScreen);
#endif
}

void scale_and_flip(SDL_Surface *src_surface)
{
    if(renderer == NULL) {
        // Standard SDL framebuffer path (ESP-IDF): use window surface directly
        SDL_Surface *fb = SDL_GetWindowSurface(window);
        if(!fb) {
            SDL_Log("Renderer is NULL and no window surface available: %s", SDL_GetError());
            return;
        }

        // Convert palettized INDEX8 surface to the framebuffer's format
        SDL_Surface *converted = SDL_ConvertSurface(src_surface, fb->format);
        if(!converted) {
            SDL_Log("Failed to convert surface to framebuffer format: %s", SDL_GetError());
            return;
        }

        // Clear the framebuffer first
        SDL_FillSurfaceRect(fb, NULL, 0);

        // IMPORTANT: For ESP32-P4, do not use SDL_BlitSurface which can cause stretching
        // Instead, directly copy the pixels to the framebuffer
#ifdef CONFIG_IDF_TARGET_ESP32P4
        // Position game content at top-left corner (0,0) instead of centering
        int offset_x = 0;
        int offset_y = 0;

        // Manual pixel copy to framebuffer without any stretching
        uint8_t *src_pixels = (uint8_t *) converted->pixels;
        uint8_t *dst_pixels = (uint8_t *) fb->pixels;

        // Copy each row of the converted surface to the framebuffer at the right position
        for(int y = 0; y < converted->h; y++) {
            // Calculate positions in source and destination
            uint8_t *src_row = src_pixels + (y * converted->pitch);
            uint8_t *dst_row = dst_pixels + ((y + offset_y) * fb->pitch) + (offset_x * SDL_BYTESPERPIXEL(fb->format));

            // Copy this row (careful not to exceed framebuffer boundaries)
            if((y + offset_y) < fb->h) {
                int bytes_to_copy = converted->w * SDL_BYTESPERPIXEL(converted->format);
                memcpy(dst_row, src_row, bytes_to_copy);
            }
        }
#else
        // Create destination rect to position game at top-left without scaling
        SDL_Rect dst_rect = {
            .x = 0,
            .y = 0,
            .w = src_surface->w,  // Keep original width (320)
            .h = src_surface->h   // Keep original height (200)
        };

        // Blit/Copy the converted surface to the framebuffer at native size
        if(SDL_BlitSurface(converted, NULL, fb, &dst_rect) < 0) {
            SDL_Log("Blit to framebuffer failed: %s", SDL_GetError());
            SDL_DestroySurface(converted);
            return;
        }
#endif

        // Present the framebuffer to the display; ESP-IDF backend handles rotation/centering
        if(SDL_UpdateWindowSurface(window) == false) {
            SDL_Log("SDL_UpdateWindowSurface failed: %s", SDL_GetError());
        }

        SDL_DestroySurface(converted);

        // --- FPS Calculation ---
        frame_count++;
        uint32_t current_time = SDL_GetTicks();
        if(start_time == 0) {
            start_time = current_time;
        }
        uint32_t elapsed_time = current_time - start_time;
        if(elapsed_time >= 5000) {
            float fps = (frame_count / (elapsed_time / 1000.0f));
            printf("FPS: %.2f\n", fps);
            frame_count = 0;
            start_time = current_time;
        }
        return;
    }

    // Renderer path (desktop or when a renderer is available)

    // Convert the SDL_Surface to an SDL_Texture
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, src_surface);
    if(!texture) {
        SDL_Log("Failed to create texture: %s", SDL_GetError());
        return;
    }

    // Clear the renderer
    SDL_RenderClear(renderer);

    // Get the window dimensions
    int window_width, window_height;
    SDL_GetWindowSize(window, &window_width, &window_height);

#ifdef CONFIG_IDF_TARGET_ESP32P4
    // ESP32-P4: Position game content at top-left corner at native resolution (no scaling)
    SDL_FRect dst_rect = (SDL_FRect) {0, 0, src_surface->w, src_surface->h};
#else
    // Other targets: Scale to fit the window (original behavior)
    SDL_FRect dst_rect = (SDL_FRect) {0, 0, window_width, window_height};
#endif

    // Copy the texture to the renderer
    SDL_RenderTexture(renderer, texture, NULL, &dst_rect);

    // Present the renderer (equivalent to SDL_Flip in SDL2)
    SDL_RenderPresent(renderer);

    // Cleanup: destroy the texture after rendering
    SDL_DestroyTexture(texture);

    // --- FPS Calculation ---
    frame_count++;                           // Increment the frame count
    uint32_t current_time = SDL_GetTicks();  // Get current time in milliseconds

    if(start_time == 0) {
        // Initialize the start time for the first frame
        start_time = current_time;
    }

    uint32_t elapsed_time = current_time - start_time;  // Calculate elapsed time in milliseconds

    if(elapsed_time >= 5000) {                                 // If 5 seconds have passed
        float fps = (frame_count / (elapsed_time / 1000.0f));  // Calculate FPS
        printf("FPS: %.2f\n", fps);                            // Print FPS to console

        // Reset for next interval
        frame_count = 0;
        start_time = current_time;
    }
}
