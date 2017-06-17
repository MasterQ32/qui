#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <SDL/SDL.h>

#ifndef LBUILD_PACKAGE
#error LBUILD_PACKAGE must be defined!
#endif

#define QUI_ROOT "file:/packages/" LBUILD_PACKAGE "/"

#define QUI_RESOURCE(filename) QUI_ROOT "share/" filename

/**
 * @brief Bitmask that masks the red color component from a @ref color_t
 */
#define COLOR_RMASK 0x00FF0000UL

/**
 * @brief Bitmask that masks the green color component from a @ref color_t
 */
#define COLOR_GMASK 0x0000FF00UL

/**
 * @brief Bitmask that masks the blue color component from a @ref color_t
 */
#define COLOR_BMASK 0x000000FFUL

/**
 * @brief Bitmask that masks the alpha component from a @ref color_t
 */
#define COLOR_AMASK 0xFF000000UL

/**
 * @brief The number of bits that is required to shift the red color component
 *        to the right to align it with the LSB.
 */
#define COLOR_RSHIFT 16

/**
 * @brief The number of bits that is required to shift the green color component
 *        to the right to align it with the LSB.
 */
#define COLOR_GSHIFT  8

/**
 * @brief The number of bits that is required to shift the blue color component
 *        to the right to align it with the LSB.
 */
#define COLOR_BSHIFT  0

/**
 * @brief The number of bits that is required to shift the alpha component
 *        to the right to align it with the LSB.
 */
#define COLOR_ASHIFT 24

/**
 * @brief Macro creating a @ref color_t value from rgb values.
 * @param r the red component
 * @param g the green component
 * @param b the blue component
 * @returns @ref color_t that contains the given color
 */
#define RGB(r,g,b) ( (((r)&0xFFUL)<<16) | (((g)&0xFFUL)<<8) | (((b)&0xFFUL)<<0) | 0xFF000000UL )

/**
 * @brief Macro creating a @ref color_t value from rgba values.
 * @param r the red component
 * @param g the green component
 * @param b the blue component
 * @param a the alpha component
 * @returns @ref color_t that contains the given color
 */
#define RGBA(r,g,b,a) ( (((r)&0xFFUL)<<16) | (((g)&0xFFUL)<<8) | (((b)&0xFFUL)<<0) |  (((a)&0xFFUL)<<24) )

/**
 * @brief Helper macro that returns a lvalue to a single pixel
 * @param bitmap A pointer to the bitmap.
 * @param x      x coordinate of the pixel.
 * @param y      y coordinate of the pixel.
 * @return writable lvalue to the pixel at (x,y)
 * @remarks This macro does not check any out-of-bound errors and will
 *          happily access invalid memory.
 */
#define PIXREF(bitmap, x, y) ((bitmap)->pixels[(bitmap)->width * (y) + (x)])

/**
 * @brief An rgba color value.
 */
typedef uint32_t color_t;

/**
 * @brief A GUI window
 **/
typedef struct quiwindow
{
	/**
	 * @brief The identifier of this window
	 */
	uint32_t const id;

	/**
	 * @brief A pointer to the shared memory of the window contents.
	 */
	color_t * const frameBuffer;

	/**
	 * @brief Width of the window surface (without decoration)
	 * @see quibitmap.pixels
	 */
	uint32_t const width;

	/**
	 * @brief Height of the window surface (without decoration)
	 */
	uint32_t const height;
} window_t;

/**
 * @brief A bitmap containing RGBA pixels
 */
typedef struct quibitmap
{
	/**
	 * @brief A pointer to the pixels of the bitmap.
	 * The pixels are stored row-major, so it is indexed by
	 * `pixels[y * width + x]`
	 */
	color_t * const pixels;

	/**
	 * @brief Width of the bitmap in pixels.
	 */
	uint32_t const width;

	/**
	 * @brief Height of the bitmap in pixels.
	 */
	uint32_t const height;

	/**
	 * @brief A set of flags for the bitmap.
	 * @remarks Mostly used by internal functions
	 */
	uint32_t const flags;
} bitmap_t;

/**
 * @brief Tries to open the UI system.
 * @return True on success, false otherwise.
 * @remarks The GUI service "gui" must be started in order to succeed.
 */
bool qui_open();


/**
 * @brief Creates a new window
 * @param width  The width of the window.
 * @param height The height of the window.
 * @param flags  A set of flags affecting the window or its creation.
 * @return Pointer to the created window or NULL on failure
 */
window_t * qui_createWindow(uint32_t width, uint32_t height, uint32_t flags);

/**
 * @brief Tells the GUI system that the content of this window has changed
 *        and a redraw is required.
 * @param window The window that should be redrawn
 */
void qui_updateWindow(window_t * window);

/**
 * @brief Destroys the window and frees its memory.
 * @param window The window to be destroyed.
 */
void qui_destroyWindow(window_t * window);

/**
 * @brief Fetches events from the event queue.
 * @param event A pointer to an SDL_Event struct that will contain the received
 *              event on success.
 * @return true when an event was fetched, false otherwise
 * @todo Change event type to something more GUI-related with the affected
 *       window included (so multiple windows aren't a problem)
 */
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
