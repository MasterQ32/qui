#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syscall.h>
#include <sleep.h>

#include <SDL/SDL_image.h>
#include <init.h>

#include "qui.h"

int main(int argc, char ** argv)
{
	stdout = NULL; // Console only!

	if(qui_open() == false) {
		printf("Failed to open gui!\n");
		exit(EXIT_FAILURE);
	}

	window_t * window = qui_createWindow(640, 480, 0);
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
				case SDL_KEYDOWN:
					printf("Key down: %d, '%c'\n", e.key.keysym.sym, e.key.keysym.unicode);
					if(e.key.keysym.sym == SDLK_e) {
						const char * args[] = {
							NULL,
						};
						init_execv("dora", args);
					}
					else if(e.key.keysym.sym == SDLK_f) {
						const char * args[] = {
							NULL,
						};
						init_execv("fontdemo", args);
					}
					else if(e.key.keysym.sym == SDLK_d) {
						const char * args[] = {
							NULL,
						};
						init_execv("demo", args);
					}
					break;
			}
		}

		if(requiresQuit) {
			break;
		}

		if(requiresPaint && !requiresQuit) {
			// Clear window
			qui_clearWindow(window, RGBA(0, 0, 0, 0));



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
