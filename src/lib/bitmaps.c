#include "qui.h"
#include "quidata.h"

#include <init.h>
#include <rpc.h>
#include <stdlib.h>

#include <png.h>

// Bitmap flag: Don't free "pixels"
#define BMP_NOFREE (1<<0)


bitmap_t * qui_getWindowSurface(window_t * window)
{
	if(window == NULL) {
		return NULL;
	}
	bitmap_t value = {
		.pixels = window->frameBuffer,
		.width = window->width,
	    .height = window->height,
		.flags = BMP_NOFREE,
	};
	bitmap_t * bmp = malloc(sizeof(bitmap_t));
	memcpy(bmp, &value, sizeof value);
	return bmp;
}

bitmap_t * qui_createBitmap(uint32_t width, uint32_t height)
{
	if(width == 0 || height == 0) {
		return NULL;
	}
	bitmap_t value = {
		.pixels = malloc(sizeof(color_t) * width * height),
	    .width = width,
	    .height = height,
	    .flags = 0,
	};
	if(value.pixels == NULL) {
		return NULL;
	}
	bitmap_t * bmp = malloc(sizeof(bitmap_t));
	memcpy(bmp, &value, sizeof value);
	return bmp;
}

bitmap_t * qui_loadBitmap(char const * fileName)
{
	if(fileName == NULL) {
		return NULL;
	}
	char header[8];    // 8 is the maximum size that can be checked

	// open file and test for it being a png
	FILE *fp = fopen(fileName, "rb");
	if (fp == NULL) {
		return NULL;
	}
	fread(header, 1, 8, fp);
	if (png_sig_cmp(header, 0, 8)) {
		fclose(fp);
		return NULL;
	}


	// initialize stuff
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (png_ptr == NULL) {
		fclose(fp);
		return NULL;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (info_ptr == NULL) {
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		fclose(fp);
		return NULL;
	}

	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		return NULL;
	}

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	int width = png_get_image_width(png_ptr, info_ptr);
	int height = png_get_image_height(png_ptr, info_ptr);
	png_byte color_type = png_get_color_type(png_ptr, info_ptr);
	png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	int number_of_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);

	// read file
	if (setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		fclose(fp);
		return NULL;
	}

	png_bytep * row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
	for (int y = 0; y < height; y++) {
		row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));
	}

	png_read_image(png_ptr, row_pointers);

	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

	fclose(fp);

	bitmap_t * bitmap = qui_createBitmap(width, height);

	if(bitmap != NULL) {
		for(int y = 0; y < height; y++) {
			png_bytep row = row_pointers[y];
			for(int x = 0; x < width; x++) {
				if(color_type == PNG_COLOR_TYPE_RGB) {
					PIXREF(bitmap, x, y) = RGB(
						row[3*x + 0],
						row[3*x + 1],
						row[3*x + 2]);

				} else {
					PIXREF(bitmap, x, y) = RGBA(
						row[4*x + 0],
						row[4*x + 1],
						row[4*x + 2],
						row[4*x + 3]);
				}

			}
		}
	}

	for(int y = 0; y < height; y++) {
		free(row_pointers[y]);
	}
	free(row_pointers);

	return bitmap;
}

void qui_destroyBitmap(bitmap_t * bitmap)
{
	if(bitmap == NULL) {
		return;
	}
	if((bitmap->flags & BMP_NOFREE) == 0) {
		free(bitmap->pixels);
	}
	free(bitmap);
}

void qui_clearBitmap(bitmap_t * bitmap, color_t color)
{
	if(bitmap == NULL) {
		return;
	}
	for(uint32_t y = 0; y < bitmap->height; y++) {
		for(uint32_t x = 0; x < bitmap->width; x++) {
			PIXREF(bitmap, x, y) = color;
		}
	}
}

void qui_blitBitmap(bitmap_t * src, bitmap_t * dest, int x, int y)
{
	qui_blitBitmapExt(src, 0, 0, dest, x, y, src->width, src->height);
}

void qui_blitBitmapExt(
	bitmap_t * src,
	int srcX, int srcY,
	bitmap_t * dest,
	int destX, int destY,
	int width, int height)
{
	if(src == NULL || dest == NULL) {
		return;
	}
	if(width <= 0 || height <= 0) {
		return;
	}
	for(int y = 0; y < height; y++) {
		for(int x = 0; x < width; x++) {
			int sx = srcX + x;
			int sy = srcY + y;
			int dx = destX + x;
			int dy = destY + y;
			if(sx < 0 || sy < 0 || dx < 0 || dy < 0) {
				continue;
			}
			if(sx >= src->width || sy >= src->height) {
				continue;
			}
			if(dx >= dest->width || dy >= dest->height) {
				continue;
			}
			color_t color = PIXREF(src, sx, sy);
			if((color & COLOR_AMASK) == 0) {
				// Alpha=0 is trivial
				continue;
			}

			if((color & COLOR_AMASK) != COLOR_AMASK) {
				// TODO: Implement fast and correct alpha blending
				int alpha = (color >> COLOR_ASHIFT);

				color_t dst = PIXREF(dest, dx, dy);
				color_t src = color;

				int sR = 0xFF & (src >> COLOR_RSHIFT);
				int sG = 0xFF & (src >> COLOR_GSHIFT);
				int sB = 0xFF & (src >> COLOR_BSHIFT);

				int dR = 0xFF & (dst >> COLOR_RSHIFT);
				int dG = 0xFF & (dst >> COLOR_GSHIFT);
				int dB = 0xFF & (dst >> COLOR_BSHIFT);

				PIXREF(dest, dx, dy) = RGB(
					(alpha * sR + (255 - alpha) * dR) / 255,
					(alpha * sG + (255 - alpha) * dG) / 255,
					(alpha * sB + (255 - alpha) * dB) / 255);

			} else {
				PIXREF(dest, dx, dy) = color;
			}
		}
	}
}
