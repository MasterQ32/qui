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
#include <sys/wait.h>
#include <kbd.h>

#include "qui.h"
#include "quidata.h"

#define WF_DIRTY (1<<0)

// #define TRACE(text) printf("%s:%d (%s): %s\n", __FILE__, __LINE__, __func__, #text);
#define TRACE(text)

struct window
{
	struct window * previous;
	struct window * next;
	uint32_t width, height, membuf;
	color_t * framebuffer;
	int top, left;
	SDL_Surface * surface;
	int flags;
	pid_t creator;
};

struct window * top = NULL;
struct window * bottom = NULL;

SDL_Rect getWindowRect(struct window * window) {
	return (SDL_Rect) {
		window->left, window->top,
		window->width + 5,
		window->height + 25,
	};
};

SDL_Rect getClientRect(struct window * window) {
	return (SDL_Rect) {
		   window->left + 2, window->top + 22,
		   window->width, window->height,
	};
}

bool rectContains(SDL_Rect const * rect, int x, int y) {
	if(x < rect->x || y < rect->y) {
		return false;
	}
	if(x >= (rect->x + rect->w) || y >= (rect->y + rect->h)) {
		return false;
	}
	return true;
}

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
	win->next = NULL;
	win->previous = NULL;
}

void bringToFront(struct window * window)
{
	// This will move the window to the front of the list
	removeWindow(window);
	insertWindow(window);
}

struct window * getWindowByPosition(int x, int y)
{
	for(struct window * win = top; win != NULL; win = win->previous) {
		SDL_Rect rect = getWindowRect(win);
		if(rectContains(&rect, x, y)) {
			return win;
		}
	}
	return NULL;
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
	window->creator = client;
	window->membuf = create_shared_memory(
		sizeof(color_t) * window->width * window->height);
	window->framebuffer = open_shared_memory(window->membuf);
	window->surface = SDL_CreateRGBSurface(
		0,
		window->width,
		window->height,
		32,
		0x00FF0000,
		0x0000FF00,
        0x000000FF,
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

	if(client != window->creator) {
		// Window access violation!
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
	if(client != window->creator) {
		// Window access violation!
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

	const SDL_Rect target = getWindowRect(window);
	if(false) // Don't draw the outline!
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

	TRACE(Blit top left);
	if(rect.x < backbuffer->w && rect.y < backbuffer->h) {
		SDL_BlitSurface(skin, &src, backbuffer, &rect);
	}

	TRACE(Blit horizontal borders);
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
		if(rect.x < backbuffer->w && rect.y < backbuffer->h) {
			SDL_BlitSurface(skin, &src, backbuffer, &rect);
		}
		rect.y = target.y + target.h - 3;
		src.y = 41;
		if(rect.x < backbuffer->w && rect.y < backbuffer->h) {
			SDL_BlitSurface(skin, &src, backbuffer, &rect);
		}
	}

	src = (SDL_Rect) {
		34, 0,
		22, 22
	};
	rect = (SDL_Rect) {
		target.x + target.w - 22, target.y,
		4, 22
	};
    if(rect.x < backbuffer->w && rect.y < backbuffer->h) {
		SDL_BlitSurface(skin, &src, backbuffer, &rect);
	}
	src = (SDL_Rect) {
		0, 41,
		4, 3
	};
	rect = (SDL_Rect) {
		target.x, target.y + target.h - 3,
		4, 3
	};
	if(rect.x < backbuffer->w && rect.y < backbuffer->h) {
		SDL_BlitSurface(skin, &src, backbuffer, &rect);
	}

	src = (SDL_Rect) {
		34, 41,
		22, 3
	};
	rect = (SDL_Rect) {
		target.x + target.w - 22, target.y + target.h - 3,
		4, 3
	};
	TRACE(Blit bottom right);
	if(rect.x < backbuffer->w && rect.y < backbuffer->h) {
		SDL_BlitSurface(skin, &src, backbuffer, &rect);
	}

	TRACE(Blit vertical borders);
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
		if(rect.x < backbuffer->w && rect.y < backbuffer->h) {
			SDL_BlitSurface(skin, &src, backbuffer, &rect);
		}
		src.x = 53;
		rect.x = target.x + target.w - 3;
		if(rect.x < backbuffer->w && rect.y < backbuffer->h) {
			SDL_BlitSurface(skin, &src, backbuffer, &rect);
		}
	}

	rect = getClientRect(window);
	TRACE(Blit content);
	SDL_BlitSurface(
		window->surface,
		NULL,
		backbuffer,
		&rect);
}

void forwardEvent(SDL_Event event, struct window * target)
{
	if(target == NULL) {
		return;
	}
	SDL_Rect rect = getClientRect(target);
	switch(event.type)
	{
		case SDL_MOUSEBUTTONDOWN:
		case SDL_MOUSEBUTTONUP:
			event.button.x -= rect.x;
			event.button.y -= rect.y;
			break;
		case SDL_MOUSEMOTION:
			event.motion.x -= rect.x;
			event.motion.y -= rect.y;
			break;
	}

	rpc_get_dword(target->creator, MSG_WINDOW_EVENT, sizeof(event), (char*)&event);
}

static pid_t kbc_pid;

static void svnKeyboardCallback(pid_t pid, uint32_t cid, size_t data_size, void* data)
{
    uint8_t* kbd_data = data;

    if ((pid != kbc_pid) || (data_size != 2)) {
        return;
    }

	printf("Got keyhit: %d %d\n", kbd_data[0], kbd_data[1]);
}

// The top-most window is always focused!
struct window * getFocus() { return top; }

int main(int argc, char** argv)
{
	// Initialize video
	if(SDL_Init(SDL_INIT_VIDEO) < 0) {
		printf("Failed to initialize SDL: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	SDL_EnableUNICODE(1);

	if(IMG_Init(IMG_INIT_PNG) < 0) {
		printf("Failed to initialize IMG: %s\n", IMG_GetError());
		exit(EXIT_FAILURE);
	}

	SDL_Surface * screen = SDL_SetVideoMode(1024, 768, 24, 0);
	if(screen == NULL) {
		printf("Failed to open video: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	// Write to serial out instead of vconsole
	stdout = NULL;

	printf("Screen Info:\n");
	printf("\tResolution: %d×%d\n", screen->w, screen->h);
	printf("\tBPP:        %d\n", screen->format->BitsPerPixel);
	printf("\tEncoding:   R=%x G=%x B=%x A=%x\n",
		   screen->format->Rmask,
	       screen->format->Gmask,
	       screen->format->Bmask,
	       screen->format->Amask);


	backbuffer = SDL_CreateRGBSurface(
		0,
		screen->w, screen->h, screen->format->BitsPerPixel,
		screen->format->Rmask,
		screen->format->Gmask,
		screen->format->Bmask,
		screen->format->Amask);

	skin = IMG_Load(QUI_RESOURCE("skin.png"));
	if(skin == NULL) {
		printf("Failed to open image: %s\n", IMG_GetError());
		exit(EXIT_FAILURE);
	}

	SDL_Surface * cursor = IMG_Load(QUI_RESOURCE("cursor.png"));
	if(cursor == NULL) {
		printf("Failed to open image: %s\n", IMG_GetError());
		exit(EXIT_FAILURE);
	}

	SDL_ShowCursor(SDL_DISABLE);

	register_message_handler(MSG_WINDOW_CREATE, svcWindowCreate);
	register_message_handler(MSG_WINDOW_DESTROY, svcWindowDestroy);
	register_message_handler(MSG_WINDOW_UPDATE, svcWindowUpdate);

	init_service_register(QUI_SVC);

	// Start dora!
	const char * args[] = {
		NULL,
	};
	pid_t dora_id = init_execv(QUI_ROOT "bin/dora", args);

	SDL_Event e;
	bool quit = false;
	int mouseX = 0, mouseY = 0;
	bool dirty = true;
	struct window * draggedWindow = NULL;
	while(!quit)
	{
		int status;
		if(get_parent_pid(dora_id) == 0) {
			// Stop the beat!
			break;
		}

		while(SDL_PollEvent(&e))
		{
			if(e.type == SDL_QUIT) quit = true;
			if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) quit = true;

			bool eatEvent = false;
			if(e.type == SDL_MOUSEBUTTONDOWN) {
				struct window * clicked = getWindowByPosition(
					e.button.x,
					e.button.y);
				if(clicked != NULL) {
					bringToFront(clicked);
					SDL_Rect target = getWindowRect(clicked);
					bool dragging = (e.button.x >= target.x + 2)
						&& (e.button.x <= target.x + target.w - 3)
						&& (e.button.y >= target.y + 3)
						&& (e.button.y <= target.y + 21);
					bool closing = (e.button.x >= target.x + target.w - 20)
					        && (e.button.x < target.x + target.w - 5)
					        && (e.button.y >= target.y + 5)
					        && (e.button.y < target.y + 19);
					if(closing) {
						removeWindow(clicked);

						SDL_Event killEvent;
						killEvent.type = SDL_QUIT;

						forwardEvent(killEvent, clicked);
						eatEvent = true;
						if(draggedWindow == clicked) {
							draggedWindow = NULL;
						}

					} else if(dragging) {
						draggedWindow = clicked;
					}
					dirty = true;
				}
			}
			if(e.type == SDL_MOUSEBUTTONUP) {
				draggedWindow = NULL;
			}
			if(e.type == SDL_MOUSEMOTION)
			{
				mouseX = e.motion.x;
				mouseY = e.motion.y;
				if(draggedWindow != NULL) {
					draggedWindow->left += e.motion.xrel;
					draggedWindow->top += e.motion.yrel;
					dirty = true;
				}
			}

			struct window * focus = getFocus();

			if(focus != NULL) {
				SDL_Rect client = getClientRect(focus);
				switch(e.type) {
					case SDL_MOUSEMOTION:
						eatEvent |= !rectContains(&client, e.motion.x, e.motion.y);
						break;
					case SDL_MOUSEBUTTONDOWN:
					case SDL_MOUSEBUTTONUP:
						eatEvent |= !rectContains(&client, e.button.x, e.button.y);
						break;
				}
				if(eatEvent == false) {
					forwardEvent(e, focus);
				}
			}
		}

		// anything dirty?
		for(struct window * it = bottom; it != NULL; it = it->next) {
			if(it->flags & WF_DIRTY) {
				dirty = true;
				break;
			}
		}

		if(dirty) {
			// Clear backbuffer
			SDL_FillRect(backbuffer, NULL, SDL_MapRGB(backbuffer->format, 0, 0x80, 0x80));

			// Draw window!
			for(struct window * it = bottom; it != NULL; it = it->next) {
				renderWindow(it);
			}

			dirty = false;
		}

		// "Clear"
		SDL_BlitSurface(backbuffer, NULL, screen, NULL);

		// Cursor
		SDL_BlitSurface(
			cursor,
			NULL,
			screen,
			&((SDL_Rect){
				mouseX, mouseY,
				16, 24
			}));

		// Render everything
		SDL_Flip(screen);

		// SDL_Delay(10);
		yield();
		// wait_for_rpc();
	}

	SDL_Quit();

	return EXIT_SUCCESS;
}
