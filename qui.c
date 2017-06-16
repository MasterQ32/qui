#include "qui.h"
#include "quidata.h"

#include <init.h>
#include <rpc.h>
#include <stdlib.h>

#include <png.h>

struct eventmsg
{
	SDL_Event event;
	struct eventmsg * next;
};

static pid_t svc = 0;

static struct eventmsg * messageQueue = NULL;
static struct eventmsg * messageQueueTop = NULL;

void cliReceiveEvent(pid_t server, uint32_t correlation_id, size_t size, void *args);

bool qui_open()
{
	if(svc != 0) {
		return true;
	}
	svc = init_service_get("gui");
	if(svc != 0) {
		register_message_handler(MSG_WINDOW_EVENT, cliReceiveEvent);
	}
	return (svc != 0);
}

// WINDOW API

window_t * qui_createWindow(uint32_t width, uint32_t height, uint32_t flags)
{
	if(width <= 0 || height <= 0) {
		return NULL;
	}
	struct wincreate_req req = {
		.width = width,
		.height = height,
	};

	response_t *respdata = rpc_get_response(svc, MSG_WINDOW_CREATE, sizeof req, (char*)&req);
	if(respdata->data_length < sizeof(struct wincreate_resp)) {
		printf("CLIENT: Got useless response!\n");
		// TODO: Release resp?
		free(respdata->data);
		free(respdata);
		return NULL;
	}
	struct wincreate_resp * resp = respdata->data;

	window_t value = {
		.id = resp->framebuf,
		.width = resp->width,
		.height = resp->height,
		.frameBuffer = open_shared_memory(resp->framebuf),
	};

	window_t * window = malloc(sizeof(window_t));
	memcpy(window, &value, sizeof value);

	free(respdata->data);
	free(respdata);

	if(window->frameBuffer == NULL) {
		printf("CLIENT: Failed to create framebuffer %d\n", window->id);
		free(window);
		return NULL;
	}

	qui_clearBitmap(qui_getWindowSurface(window), RGB(0,0,0));

	return window;
}

void qui_updateWindow(window_t * window)
{
	if(svc == 0) {
		// TODO: Maybe fail here!
		return;
	}
	if(window == NULL) {
		return;
	}
	int result = rpc_get_int(svc, MSG_WINDOW_UPDATE, sizeof window->id, (char*)&window->id);
	// TODO: Use result!
}

void qui_destroyWindow(window_t * window)
{
	if(svc == 0) {
		// TODO: Maybe fail here!
		return;
	}
	if(window == NULL) {
		return;
	}

	close_shared_memory(window->id);

	int result = rpc_get_int(svc, MSG_WINDOW_DESTROY, sizeof window->id, (char*)&window->id);
	// TODO: Use result!

	free(window);
}

bool qui_fetchEvent(SDL_Event * event)
{
	*event = ((SDL_Event) { 0 });

	if(messageQueue == NULL) {
		return false;
	}

	p();
	struct eventmsg * msg = messageQueue;
	messageQueue = messageQueue->next;
	if(messageQueue == NULL) {
		messageQueueTop = NULL;
	}
	v();

	*event = msg->event;
	free(msg);

	return true;
}

void cliReceiveEvent(pid_t server, uint32_t correlation_id, size_t size, void *args)
{
	SDL_Event * event = args;

	p();
	struct eventmsg * it = malloc(sizeof(struct eventmsg));
	it->event = *event;
	it->next = NULL;

	if(messageQueueTop == NULL) {
		messageQueue = it;
		messageQueueTop = it;
	} else {
		messageQueueTop->next = it;
		messageQueueTop = it;
	}

	v();

	rpc_send_dword_response(server, correlation_id, 0);
}



/*
void read_png_file(char* file_name)
{
	int x, y;

	int width, height;
	png_byte color_type;
	png_byte bit_depth;

	png_structp png_ptr;
	png_infop info_ptr;
	int number_of_passes;
	png_bytep * row_pointers;

	char header[8];    // 8 is the maximum size that can be checked

	// open file and test for it being a png
	FILE *fp = fopen(file_name, "rb");
	if (!fp)
			abort_("[read_png_file] File %s could not be opened for reading", file_name);
	fread(header, 1, 8, fp);
	if (png_sig_cmp(header, 0, 8))
			abort_("[read_png_file] File %s is not recognized as a PNG file", file_name);


	// initialize stuff
	png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

	if (!png_ptr)
			abort_("[read_png_file] png_create_read_struct failed");

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
			abort_("[read_png_file] png_create_info_struct failed");

	if (setjmp(png_jmpbuf(png_ptr)))
			abort_("[read_png_file] Error during init_io");

	png_init_io(png_ptr, fp);
	png_set_sig_bytes(png_ptr, 8);

	png_read_info(png_ptr, info_ptr);

	width = png_get_image_width(png_ptr, info_ptr);
	height = png_get_image_height(png_ptr, info_ptr);
	color_type = png_get_color_type(png_ptr, info_ptr);
	bit_depth = png_get_bit_depth(png_ptr, info_ptr);

	number_of_passes = png_set_interlace_handling(png_ptr);
	png_read_update_info(png_ptr, info_ptr);


	// read file
	if (setjmp(png_jmpbuf(png_ptr)))
			abort_("[read_png_file] Error during read_image");

	row_pointers = (png_bytep*) malloc(sizeof(png_bytep) * height);
	for (y=0; y<height; y++)
			row_pointers[y] = (png_byte*) malloc(png_get_rowbytes(png_ptr,info_ptr));

	png_read_image(png_ptr, row_pointers);

	fclose(fp);
}
*/

// GRAPHICS API


bitmap_t * qui_getWindowSurface(window_t * window)
{
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

bitmap_t * qui_newBitmap(uint32_t width, uint32_t height)
{
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

bitmap_t * qui_loadBitmap(char const * fileName);

void qui_destroyBitmap(bitmap_t * bitmap)
{
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

void qui_blitBitmap(bitmap_t * src, bitmap_t * dest, int x, int y);

void qui_blitBitmapExt(
	bitmap_t * src,
	bitmap_t * dest,
	int srcX, int srcY,
	int destX, int destY,
	int width, int height);
