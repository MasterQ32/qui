#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <SDL/SDL.h>

#define COLOR_RMASK 0x00FF0000UL
#define COLOR_GMASK 0x0000FF00UL
#define COLOR_BMASK 0x000000FFUL
#define COLOR_AMASK 0xFF000000UL

#define COLOR_RSHIFT 16
#define COLOR_GSHIFT  8
#define COLOR_BSHIFT  0
#define COLOR_ASHIFT 24

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

/**
 * @brief Gets a drawable bitmap that will represent the window contents.
 * @param window The window for which the surface should be returned
 * @return bitmap that references the window surface or NULL on failure
 * @remark The bitmap gets invalid when the window is destroyed. It must be
 *         freed still.
 */
bitmap_t * qui_getWindowSurface(window_t * window);

/**
 * @brief Creates a new bitmap.
 * @param width The horizontal size of the bitmap
 * @param height The vertical size of the bitmap
 * @return The bitmap that was created or NULL on failure.
 */
bitmap_t * qui_createBitmap(uint32_t width, uint32_t height);

/**
 * @brief Loads a PNG file and returns a bitmap containing the image.
 * @param fileName The file name of the PNG file.
 * @return A bitmap containing the loaded image.
 */
bitmap_t * qui_loadBitmap(char const * fileName);

/**
 * @brief Destroys the bitmap and releases its memory.
 * @param bitmap The bitmap to be freed.
 */
void qui_destroyBitmap(bitmap_t * bitmap);

/**
 * @brief Clears the bitmap to the given color.
 * @param bitmap The bitmap to be cleared
 * @param color  The color which will fill the bitmap afterwards
 */
void qui_clearBitmap(bitmap_t * bitmap, color_t color);

/**
 * @brief Copies one bitmap into another.
 * @param src  The source that will be copied
 * @param dest The destination that will receive the copy
 * @param x    x-coordinate on the destination
 * @param y    y-coordinate on the destination
 * @remarks The source bitmap will be copied completly into the destination
 * @remarks Alpha blending will be used when blitting
 */
void qui_blitBitmap(bitmap_t * src, bitmap_t * dest, int x, int y);

/**
 * @brief Copies a portion of a bitmap into another with extended settings.
 * @param src    The source bitmap that will be copied
 * @param srcX   The origin of the rectangle on the source bitmap. x-coordinate.
 * @param srcY   The origin of the rectangle on the source bitmap. y-coordinate.
 * @param dest   The destination bitmap
 * @param destX  The target position of the rectangle on the destination bitmap.
 *               x-coordinate.
 * @param destY  The target position of the rectangle on the destination bitmap.
 *               y-coordinate.
 * @param width  The width of the rectangle
 * @param height The height of the rectangle.
 * @remarks Alpha blending will be used when blitting
 */
void qui_blitBitmapExt(
	bitmap_t * src,
	int srcX, int srcY,
	bitmap_t * dest,
	int destX, int destY,
	int width, int height);
