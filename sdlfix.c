#define _GNU_SOURCE
#include <assert.h>
#include <dlfcn.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SDL_HWSURFACE 0x00000001
#define SDL_FULLSCREEN	0x80000000
#define SDL_DOUBLEBUF	0x40000000
#define SDL_SWSURFACE	0x00000000

typedef struct SDLPixelFormat
{
  void *palette;
  uint8_t BitsPerPixel;
  uint8_t BytesPerPixel;
  uint8_t Rloss, Gloss, Bloss, Aloss;
  uint8_t Rshift, Gshift, Bshift, Ashift;
  uint32_t Rmask, Gmask, Bmask, Amask;
  uint32_t colorkey;
  uint8_t alpha;
} SDLPixelFormat;

typedef struct
{
  uint32_t hw_available : 1;
  uint32_t wm_available : 1;
  uint32_t blit_hw : 1;
  uint32_t blit_hw_CC : 1;
  uint32_t blit_hw_A : 1;
  uint32_t blit_sw : 1;
  uint32_t blit_sw_CC : 1;
  uint32_t blit_sw_A : 1;
  uint32_t blit_fill : 1;
  uint32_t video_mem;
  void *vfmt;
  int current_w;
  int current_h;
} SDLVideoInfo;

typedef struct SDLSurface
{
  uint32_t flags;
  SDLPixelFormat *format;
  int w, h;
  int pitch;
  void *pixels;
  void *userdata;
  int locked;
  void *lock_data;

} SDLSurface;

typedef struct SDLRect {
    signed short x, y;
    unsigned short w, h;
} SDLRect;


static int (*real_SDL_Init)(int) = NULL;
static void (*real_SDL_UpdateRects)(SDLSurface*, int, SDLRect*) = NULL;
static void *(*real_SDL_SetVideoMode)(int, int, int, uint32_t) = NULL;
static void *(*real_SDL_CreateRGBSurface)(uint32_t, int, int, int, uint32_t, uint32_t, uint32_t, uint32_t) = NULL;

static SDLSurface *screen = NULL;
static SDLSurface *realScreen = NULL;
static int debug = 0;

int SDL_Init(int flags) {
    if(getenv("SDLFIX_DEBUG")) debug = 1;
    fprintf(stderr, "sdlfix: override SDL screen functions for RS97 (debug=%d)\n", debug);

    if (real_SDL_Init == NULL)
    {
        real_SDL_Init = dlsym(RTLD_NEXT, "SDL_Init");
        if(debug) fprintf(stderr, "SDL_Init = %p\n", real_SDL_Init);
        assert(real_SDL_Init != NULL);
    }

    if (real_SDL_SetVideoMode == NULL)
    {
        real_SDL_SetVideoMode = dlsym(RTLD_NEXT, "SDL_SetVideoMode");
        if(debug) fprintf(stderr, "SDL_SetVideoMode = %p\n", real_SDL_SetVideoMode);
        assert(real_SDL_SetVideoMode != NULL);
    }

    if (real_SDL_CreateRGBSurface == NULL)
    {
        real_SDL_CreateRGBSurface = dlsym(RTLD_NEXT, "SDL_CreateRGBSurface");
        if(debug) fprintf(stderr, "SDL_CreateRGBSurface = %p\n", real_SDL_CreateRGBSurface);
        assert(real_SDL_CreateRGBSurface != NULL);
    }

    if (real_SDL_UpdateRects == NULL)
    {
        real_SDL_UpdateRects = dlsym(RTLD_NEXT, "SDL_UpdateRects");
        if(debug) fprintf(stderr, "SDL_UpdateRects = %p\n", real_SDL_UpdateRects);
        assert(real_SDL_UpdateRects != NULL);
    }

    return real_SDL_Init(flags);
}

void SDL_UpdateRects(SDLSurface *surface, int numrects, SDLRect *rects) {
    if(debug) fprintf(stderr, "call to SDL_UpdateRects(%p, %d, %p)\n", surface, numrects, rects);
    if(surface == screen) {
        SDLRect mapped_rects[numrects];
        for(int r = 0; r < numrects; r++) {
            int x = rects[r].x, y = rects[r].y, w = rects[r].w, h = rects[r].h;
            if(debug) fprintf(stderr, "  %d: {x=%d, y=%d, w=%d, h=%d}\n", r, x, y, w, h); 
            mapped_rects[r].x = x;
            mapped_rects[r].y = y * 2;
            mapped_rects[r].w = w;
            mapped_rects[r].h = h * 2;
            if(x < 0) 
            {
                w += x;
                x = 0;
            }
            if(x + w > realScreen->w) 
            {
                w = realScreen->w - x;
            }
            for(int j = 0; j < h; j++) 
            {
                if(y + j >= 0 && y + j < screen->h) 
                {
                    int width = w * realScreen->format->BytesPerPixel;

                    memcpy(((uint8_t *)realScreen->pixels) + ((j + y) * 2) * realScreen->pitch + x * realScreen->format->BytesPerPixel,
                            ((uint8_t *)screen->pixels) + (j + y) * screen->pitch + x * screen->format->BytesPerPixel, 
                            width);
                    memcpy(((uint8_t *)realScreen->pixels) + ((j + y) * 2 + 1) * realScreen->pitch + x * realScreen->format->BytesPerPixel, 
                            ((uint8_t *)screen->pixels) + (j + y) * screen->pitch + x * screen->format->BytesPerPixel, 
                            width);
                }
            }
        }
        real_SDL_UpdateRects(realScreen, numrects, mapped_rects);
    } 
    else 
    {
        real_SDL_UpdateRects(surface, numrects, rects);
    }
}

void *SDL_SetVideoMode(int width, int height, int bitsperpixel, uint32_t flags)
{
    if(debug) fprintf(stderr, "call to SDL_SetVideoMode(%d, %d, %u)\n", width, height, flags);

    screen = (SDLSurface *) real_SDL_CreateRGBSurface(0, 320, 240, bitsperpixel, 0, 0, 0, 0);
    realScreen = (SDLSurface *) real_SDL_SetVideoMode(320, 480, bitsperpixel, SDL_FULLSCREEN | SDL_SWSURFACE | SDL_DOUBLEBUF);
    return screen;
}

