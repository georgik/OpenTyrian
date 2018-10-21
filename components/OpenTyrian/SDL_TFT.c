#include "SDL_TFT.h"



#define SPI_BUS TFT_VSPI_HOST

SDL_Surface* primary_surface;

JE_byte *** allocateTwoDimenArrayOnHeapUsingMalloc(int row, int col)
{
	JE_byte ***ptr = malloc(row * sizeof(*ptr) + row * (col * sizeof **ptr) );

	int * const data = ptr + row;
	for(int i = 0; i < row; i++)
		ptr[i] = data + i * col;

	return ptr;
}

void SDL_WM_SetCaption(const char *title, const char *icon)
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

char *SDL_GetKeyName(SDLKey key)
{
    return (char *)"";
}

SDL_Keymod SDL_GetModState(void)
{
    return (SDL_Keymod)0;
}

Uint32 SDL_GetTicks(void)
{
    return esp_timer_get_time() / 1000;    
}

int SDL_Init(Uint32 flags)
{
    return 0;
}

void SDL_Quit(void)
{

}

Uint32 SDL_WasInit(Uint32 flags)
{
    //return (tft == NULL);
	return 0;
}

int SDL_InitSubSystem(Uint32 flags)
{
    if(flags == SDL_INIT_VIDEO)
    {
    	spi_lcd_init();
    }
    return 0; // 0 = OK, -1 = Error
}

SDL_Surface *SDL_CreateRGBSurface(Uint32 flags, int width, int height, int depth, Uint32 Rmask, Uint32 Gmask, Uint32 Bmask, Uint32 Amask)
{
    SDL_Surface *surface = (SDL_Surface *)calloc(1, sizeof(SDL_Surface));
    SDL_Rect rect = { .x=0, .y=0, .w=width, .h=height};
    SDL_Color col = {.r=0, .g=0, .b=0, .unused=0};
    SDL_Palette pal =  {.ncolors=1, .colors=&col};
    SDL_PixelFormat* pf = (SDL_PixelFormat*)calloc(1, sizeof(SDL_PixelFormat));
	pf->palette = &pal;
	pf->BitsPerPixel = 8;
	pf->BytesPerPixel = 1;
	pf->Rloss = 0; pf->Gloss = 0; pf->Bloss = 0; pf->Aloss = 0,
	pf->Rshift = 0; pf->Gshift = 0; pf->Bshift = 0; pf->Ashift = 0;
	pf->Rmask = 0; pf->Gmask = 0; pf->Bmask = 0; pf->Amask = 0;
	pf->colorkey = 0;
	pf->alpha = 0;

    surface->flags = flags;
    surface->format = pf;
    surface->w = width;
    surface->h = height;
    surface->pitch = width*(depth/8);
    surface->clip_rect = rect;
    surface->refcount = 1;
    surface->pixels = heap_caps_malloc(width*height*1, MALLOC_CAP_SPIRAM);
    if(primary_surface == NULL)
    	primary_surface = surface;
    return surface;
}

int SDL_FillRect(SDL_Surface *dst, SDL_Rect *dstrect, Uint32 color)
{
    if(dst == NULL )//|| dst->sprite == NULL)
    {
        // Draw directly on screen
    	//if(dstrect == NULL)
    		//TFT_fillWindow(TFT_BLACK);
    	//else
    		//TFT_fillRect(dstrect->x, dstrect->y, dstrect->w, dstrect->h, TFT_BLACK);
    } else {
    	if(dstrect != NULL)
    	{
			for(int y = dstrect->y; y < dstrect->y + dstrect->h;y++)
				memset((unsigned char *)dst->pixels + y*320 + dstrect->x, (unsigned char)color, dstrect->w);
    	} else {
    		memset(dst->pixels, (unsigned char)color, dst->pitch*dst->h);
    	}
    }
    return 0;
}

SDL_Surface *SDL_GetVideoSurface(void)
{
    return primary_surface;
}

Uint32 SDL_MapRGB(SDL_PixelFormat *fmt, Uint8 r, Uint8 g, Uint8 b)
{
    if(fmt->BitsPerPixel == 16)
    {
        uint16_t bb = (b >> 3) & 0x1f;
        uint16_t gg = ((g >> 2) & 0x3f) << 5;
        uint16_t rr = ((r >> 3) & 0x1f) << 11;
        return (Uint32) (rr | gg | bb);        
    }
    return (Uint32)0;
}

int SDL_SetColors(SDL_Surface *surface, SDL_Color *colors, int firstcolor, int ncolors)
{
	for(int i = firstcolor; i < firstcolor+ncolors; i++)
	{
		int v=((colors[i].r>>3)<<11)+((colors[i].g>>2)<<5)+(colors[i].b>>3);
		lcdpal[i]=(v>>8)+(v<<8);
	}
	return 1;
}

SDL_Surface *SDL_SetVideoMode(int width, int height, int bpp, Uint32 flags)
{
	return SDL_GetVideoSurface();
}

void SDL_FreeSurface(SDL_Surface *surface)
{
    free(surface->pixels);
    free(surface->format);
    surface->refcount = 0;
}

void SDL_QuitSubSystem(Uint32 flags)
{

}

int SDL_Flip(SDL_Surface *screen)
{
	spi_lcd_send_boarder(screen->pixels, 20);
	//spi_lcd_send(screen->pixels);
	return 0;
}

int SDL_VideoModeOK(int width, int height, int bpp, Uint32 flags)
{
	if(bpp == 8)
		return 1;
	return 0;
}
