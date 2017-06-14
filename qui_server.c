#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <syscall.h>
#include <types.h>
#include <unistd.h>
#include <stdbool.h>

#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include <rpc.h>
#include <init.h>
#include <syscall.h>

#include "qui.h"
#include "quidata.h"

#define WF_DIRTY (1<<0)

struct window
{
	struct window * previous;
	struct window * next;
	uint32_t width, height, membuf;
	color_t * framebuffer;
	int top, left;
	SDL_Surface * surface;
	int flags;
};

struct window * top = NULL;
struct window * bottom = NULL;

struct window * getWindow(uint32_t id)
{
	// TODO: Lock here!
	for(struct window * win = bottom; win != NULL; win = win->next) {
		if(win->membuf == id) {
			return win;
		}
	}
	return NULL;
}

void insertWindow(struct window * win)
{
	// TODO: Lock here!
	if(top != NULL) {
		top->next = win;
		win->previous = top;
		top = win;
	} else {
		top = win;
		bottom = win;
	}
}

void removeWindow(struct window * win)
{
	if(win->previous != NULL) {
		win->previous->next = win->next;
	}
	if(win->next != NULL) {
		win->next->previous = win->previous;
	}
	if(top == win) {
		top = win->previous;
	}
	if(bottom == win) {
		bottom = win->next;
	}
}

void svcWindowCreate(pid_t client, uint32_t correlation_id, size_t size, void *args)
{
	struct wincreate_req * req = args;

	struct window * window = malloc(sizeof(struct window));
	window->flags = WF_DIRTY;
	window->top = 16;
	window->left = 16;
	window->width = req->width;
	window->height = req->height;
	window->previous = NULL;
	window->next = NULL;
	window->membuf = create_shared_memory(
		sizeof(color_t) * window->width * window->height);
	window->framebuffer = open_shared_memory(window->membuf);
	window->surface = SDL_CreateRGBSurface(
		0,
		window->width,
		window->height,
		32,
		0x000000FF,
		0x0000FF00,
		0x00FF0000,
	    0xFF000000);

	printf("SERVER: Created window %d×%d, %d, %p, %p\n",
		window->width,
		window->height,
		window->membuf,
		window->framebuffer,
		window->surface);

	insertWindow(window);

	struct wincreate_resp resp;
	resp.framebuf = window->membuf;
	resp.width = window->width;
	resp.height = window->height;
	rpc_send_response(client, correlation_id, sizeof(struct wincreate_resp), (char*)&resp);
}

void svcWindowDestroy(pid_t client, uint32_t correlation_id, size_t size, void * args)
{
	uint32_t wid = *((uint32_t*)args);
	struct window * window = getWindow(wid);
	if(window == NULL) {
		rpc_send_int_response(client, correlation_id, 1);
		return;
	}

	removeWindow(window);

	printf("SERVER: Destroyed window %d\n", window->membuf);

	close_shared_memory(window->membuf);
	SDL_FreeSurface(window->surface);
	free(window);

	rpc_send_int_response(client, correlation_id, 0);
}

void svcWindowUpdate(pid_t client, uint32_t correlation_id, size_t size, void * args)
{
	uint32_t wid = *((uint32_t*)args);
	struct window * window = getWindow(wid);
	if(window == NULL) {
		rpc_send_int_response(client, correlation_id, 1);
		return;
	}

	printf("SERVER: Update window %d!\n", window->membuf);
	window->flags |= WF_DIRTY;

	rpc_send_int_response(client, correlation_id, 0);
}

SDL_Surface * backbuffer;
SDL_Surface * skin;

void renderWindow(struct window * window)
{
	if(backbuffer == NULL || skin == NULL) {
		return;
	}

	if(window->flags & WF_DIRTY) {
		if(SDL_LockSurface(window->surface) < 0) {
			printf("SERVER: Failed to lock surface: %s\n", SDL_GetError());
			exit(EXIT_FAILURE);
		}

		memcpy(
			window->surface->pixels,
			window->framebuffer,
			sizeof(color_t) * window->width * window->height);

		SDL_UnlockSurface(window->surface);
		window->flags &= ~WF_DIRTY;
	}

	const SDL_Rect target = {
		window->left, window->top,
	    window->width + 5,
	    window->height + 25,
	};
	{
		SDL_Rect outline = target;
		outline.x -= 1;
		outline.y -= 1;
		outline.w += 2;
		outline.h += 2;
		SDL_FillRect(backbuffer, &outline, SDL_MapRGB(backbuffer->format, 0xFF, 0x00, 0x00));
	}
	SDL_Rect filler = target;
	SDL_FillRect(backbuffer, &filler, SDL_MapRGB(backbuffer->format, 0xC0, 0xC0, 0xC0));

	SDL_Rect rect, src;
	src = (SDL_Rect) {
		0, 0,
		4, 22
	};
	rect = (SDL_Rect) {
		target.x, target.y,
		4, 22
	};
	SDL_BlitSurface(skin, &src, backbuffer, &rect);

	for(int i = 4; i < (target.w - 22); i += 22)
	{
		src = (SDL_Rect) {
			4, 0,
			22, 22
		};
		rect = (SDL_Rect) {
			target.x + i, target.y,
			22, 22
		};
		if(rect.x + rect.w > target.w - 22)
		{
			rect.w = target.w - i;
		}
		SDL_BlitSurface(skin, &src, backbuffer, &rect);
		rect.y = target.y + target.h - 3;
		src.y = 41;
		SDL_BlitSurface(skin, &src, backbuffer, &rect);
	}

	src = (SDL_Rect) {
		34, 0,
		22, 22
	};
	rect = (SDL_Rect) {
		target.x + target.w - 22, target.y,
		4, 22
	};
	SDL_BlitSurface(skin, &src, backbuffer, &rect);

	src = (SDL_Rect) {
		0, 41,
		4, 3
	};
	rect = (SDL_Rect) {
		target.x, target.y + target.h - 3,
		4, 3
	};
	SDL_BlitSurface(skin, &src, backbuffer, &rect);

	src = (SDL_Rect) {
		34, 41,
		22, 3
	};
	rect = (SDL_Rect) {
		target.x + target.w - 22, target.y + target.h - 3,
		4, 3
	};
	SDL_BlitSurface(skin, &src, backbuffer, &rect);

	for(int i = 22; i < (target.h - 3); i += rect.h)
	{
		src = (SDL_Rect) {
			0, 22,
			3, 19
		};
		rect = (SDL_Rect) {
			target.x, target.y + i,
			src.w, src.h
		};
		if(rect.y + rect.h > (target.y + target.h - 3))
		{
			rect.h = target.h - 3 - i;
			src.h = rect.h;
		}

		src.x = 0;
		rect.x = target.x;
		SDL_BlitSurface(skin, &src, backbuffer, &rect);
		src.x = 53;
		rect.x = target.x + target.w - 3;
		SDL_BlitSurface(skin, &src, backbuffer, &rect);
	}

	rect = (SDL_Rect) {
		target.x + 2, target.y + 22,
		window->width, window->height,
	};
	SDL_BlitSurface(
		window->surface,
		NULL,
		backbuffer,
		&rect);
}

int main(int argc, char** argv)
{
	// Write to serial out instead of vconsole
	stdout = NULL;

	// video_init();

	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("Failed to initialize SDL: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	if(IMG_Init(IMG_INIT_PNG) < 0) {
		printf("Failed to initialize IMG: %s\n", IMG_GetError());
		exit(EXIT_FAILURE);
	}

	backbuffer = SDL_SetVideoMode(1024, 768, 24, 0);
	if(backbuffer == NULL) {
		printf("Failed to open video: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	skin = IMG_Load("skin.png");
	if(skin == NULL) {
		printf("Failed to open image: %s\n", IMG_GetError());
		exit(EXIT_FAILURE);
	}

	SDL_Surface * cursor = IMG_Load("cursor.png");
	if(cursor == NULL) {
		printf("Failed to open image: %s\n", IMG_GetError());
		exit(EXIT_FAILURE);
	}

	SDL_ShowCursor(SDL_DISABLE);

	init_service_register("gui");

	register_message_handler(MSG_WINDOW_CREATE, svcWindowCreate);
	register_message_handler(MSG_WINDOW_DESTROY, svcWindowDestroy);
	register_message_handler(MSG_WINDOW_UPDATE, svcWindowUpdate);

	SDL_Event e;
	bool quit = false;
	int mouseX = 0, mouseY = 0;
	bool dragging = false;
	while(!quit)
	{
		while(SDL_PollEvent(&e))
		{
			if(e.type == SDL_QUIT) quit = true;
			if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) quit = true;

			/*
			if(e.type == SDL_MOUSEBUTTONDOWN) {
				printf("x,y=(%d,%d)\n", e.button.x, e.button.y);
				dragging = (e.button.x >= target.x + 2)
					&& (e.button.x <= target.x + target.w - 3)
					&& (e.button.y >= target.y + 3)
					&& (e.button.y <= target.y + 21);
			}
			if(e.type == SDL_MOUSEBUTTONUP) {
				dragging = false;
			}
			if(e.type == SDL_MOUSEMOTION)
			{
				mouseX = e.motion.x;
				mouseY = e.motion.y;
				if(dragging) {
					target.x += e.motion.xrel;
					target.y += e.motion.yrel;
				}
			}
			*/
		}

		// anything dirty?
		bool dirty = false;
		for(struct window * it = bottom; it != NULL; it = it->next) {
			if(it->flags & WF_DIRTY) {
				dirty = true;
				break;
			}
		}

		if(dirty) {
			// Clear screen
			SDL_FillRect(backbuffer, NULL, SDL_MapRGB(backbuffer->format, 0, 0x80, 0x80));

			// Draw window!
			for(struct window * it = bottom; it != NULL; it = it->next) {
				renderWindow(it);
			}

			SDL_BlitSurface(
				cursor,
				NULL,
				backbuffer,
				&((SDL_Rect){
					mouseX, mouseY,
					16, 24
				}));

			// Render everything
			SDL_Flip(backbuffer);
		}
		SDL_Delay(10);
	}

	SDL_Quit();

	return EXIT_SUCCESS;
}
