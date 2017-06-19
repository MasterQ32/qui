#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syscall.h>
#include <sleep.h>

#include <SDL/SDL_image.h>
#include <init.h>

#include "qui.h"

struct program
{
	bitmap_t * icon;
	char const * name;
	char const * file;
	struct program * next;
};

typedef struct program program_t;

program_t * programs = NULL;

program_t * addProgram(char const * name, char const * file, char const * iconFile)
{
	program_t * pgm = malloc(sizeof(program_t));
	pgm->name = strdup(name);
	pgm->file = strdup(file);
	pgm->icon = qui_loadBitmap(iconFile);

	pgm->next = programs;
	programs = pgm;
	return pgm;
}

int main(int argc, char ** argv)
{
	if(qui_open() == false) {
		printf("Failed to open gui!\n");
		exit(EXIT_FAILURE);
	}
	stdout = NULL; // Console only!

	addProgram("Font Demo", QUI_ROOT "bin/fontdemo", QUI_RESOURCE("fontdemo.png"));
	addProgram("Demo", QUI_ROOT "bin/demo", QUI_RESOURCE("demo.png"));
	addProgram("QTerm", QUI_ROOT "bin/qterm", QUI_RESOURCE("qterm.png"));
	addProgram("Dora", QUI_ROOT "bin/dora", QUI_RESOURCE("dora.png"));

	int size = 0;
	for(program_t * it = programs; it  != NULL; it = it->next) {
		++size;
	}

	window_t * window = qui_createWindow(48, 48 * size, 0);
	if(window == NULL) {
		printf("Failed to create window!\n");
		exit(EXIT_FAILURE);
	}

	bool requiresPaint = true; // Initially, draw a frame!
	bool requiresQuit = false;
	bool dragging = false;
	while(requiresQuit == false)
	{
		SDL_Event e;
		while(qui_fetchEvent(&e)) {
			switch(e.type) {
				case SDL_QUIT:
					requiresQuit = true;
					break;
				case SDL_MOUSEBUTTONDOWN: {
					if(e.button.x < 8 || e.button.x >= 40) {
						break;
					}
					int index = e.button.y / 48;
					int offset = e.button.y % 48;
					if(offset < 8 || offset >= 40) {
						break;
					}

					program_t * it = programs;
					while(it != NULL && index > 0) {
						it = it->next;
						index--;
					}

					if(it != NULL) {
						const char * args[] = {
							NULL,
						};
						init_execv(it->file, args);
					}
					break;
				}
			}
		}

		if(requiresQuit) {
			break;
		}

		if(requiresPaint && !requiresQuit) {
			bitmap_t * surface = qui_getWindowSurface(window);
			// Clear window
			qui_clearBitmap(surface, RGBA(0, 0, 0, 0));

			int y = 8;
			for(program_t * it = programs; it != NULL; it = it->next) {
				qui_blitBitmap(
					it->icon,
					surface,
					8,
					y);
				y += 48;
			}

			// Send the graphics to the "server"
			qui_updateWindow(window);
			requiresPaint = false;
		}

		wait_for_rpc();
	}

	printf("CLIENT: Shutdown!\n");

	qui_destroyWindow(window);

	return EXIT_SUCCESS;
}
