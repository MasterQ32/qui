#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <SDL/SDL.h>

#define RGB(r,g,b) ( (((r)&0xFFUL)<<16) | (((g)&0xFFUL)<<8) | (((b)&0xFFUL)<<0) | 0xFF000000UL )
#define RGBA(r,g,b,a) ( (((r)&0xFFUL)<<16) | (((g)&0xFFUL)<<8) | (((b)&0xFFUL)<<0) |  (((a)&0xFFUL)<<24) )

#define BMP_NOFREE (1<<0)

#define PIXREF(bitmap, x, y) ((bitmap)->pixels[(bitmap)->width * (y) + (x)])

typedef uint32_t color_t;

typedef struct quiwindow
{
	// Created by RPC
	uint32_t const id;
	color_t * const frameBuffer;
	uint32_t const width;
	uint32_t const height;
} window_t;

typedef struct quibitmap
{
	color_t * const pixels;
	uint32_t const width;
	uint32_t const height;
	uint32_t const flags;
} bitmap_t;

bool qui_open();

// Window API

window_t * qui_createWindow(uint32_t width, uint32_t height, uint32_t flags);

void qui_updateWindow(window_t * window);

void qui_destroyWindow(window_t * window);

bool qui_fetchEvent(SDL_Event * event);

// Graphics API

bitmap_t * qui_getWindowSurface(window_t * window);

bitmap_t * qui_newBitmap(uint32_t width, uint32_t height);

bitmap_t * qui_loadBitmap(char const * fileName);

void qui_destroyBitmap(bitmap_t * bitmap);

void qui_clearBitmap(bitmap_t * window, color_t color);

void qui_blitBitmap(bitmap_t * src, bitmap_t * dest, int x, int y);

void qui_blitBitmapExt(
	bitmap_t * src,
	bitmap_t * dest,
	int srcX, int srcY,
	int destX, int destY,
	int width, int height);
