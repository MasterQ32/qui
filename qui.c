#include "qui.h"
#include "quidata.h"

#include <init.h>
#include <rpc.h>
#include <stdlib.h>

static pid_t svc = 0;

bool qui_open()
{
	svc = init_service_get("gui");
	return (svc != 0);
}

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

	window_t * window = malloc(sizeof(window_t));
	window->id = resp->framebuf;
	window->width = resp->width;
	window->height = resp->height;
	window->frameBuffer = open_shared_memory(window->id);

	free(respdata->data);
	free(respdata);

	if(window->frameBuffer == NULL) {
		printf("CLIENT: Failed to create framebuffer %d\n", resp->framebuf);
		return NULL;
	}

	qui_clearWindow(window, RGB(0,0,0));

	return window;
}

void qui_clearWindow(window_t * window, color_t color)
{
	if(svc == 0) {
		// TODO: Maybe fail here!
		return;
	}
	if(window == NULL) {
		return;
	}
	for(uint32_t y = 0; y < window->height; y++) {
		for(uint32_t x = 0; x < window->width; x++) {
			window->frameBuffer[window->width * y + x] = color;
		}
	}
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

	window->frameBuffer = NULL;
	close_shared_memory(window->id);

	int result = rpc_get_int(svc, MSG_WINDOW_DESTROY, sizeof window->id, (char*)&window->id);
	// TODO: Use result!

	free(window);
}
