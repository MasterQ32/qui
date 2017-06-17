#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syscall.h>
#include <sleep.h>

#include <SDL/SDL_image.h>

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

	bitmap_t * greenQuad = qui_newBitmap(32, 32);
	qui_clearBitmap(greenQuad, RGBA(0, 255, 0, 128));

	int posX = window->width / 2, posY = window->height / 2;
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
				case SDL_MOUSEBUTTONDOWN:
					posX = e.button.x;
					posY = e.button.y;
					requiresPaint = true;
					dragging = true;
					break;
				case SDL_MOUSEMOTION:
					if(dragging) {
						posX = e.button.x;
						posY = e.button.y;
						requiresPaint = true;
					}
					break;
				case SDL_MOUSEBUTTONUP:
					dragging = false;
					break;
			}
		}

		if(requiresQuit) {
			break;
		}

		if(requiresPaint && !requiresQuit) {
			bitmap_t * surface = qui_getWindowSurface(window);
			// Clear window
			qui_clearBitmap(surface, RGBA(0, 0, 0, 0));
			for(int i = -10; i <= 10; i++)
			{
				int x, y;
				x = posX + i;
				y = posY;
				if(x >= 0 && y >= 0 && x < surface->width && y < surface->width) {
					PIXREF(surface, x, y) = RGB(255, 0, 0);
				}
				x = posX;
				y = posY + i;
				if(x >= 0 && y >= 0 && x < surface->width && y < surface->width) {
					PIXREF(surface, x, y) = RGB(255, 0, 0);
				}
			}

			qui_blitBitmap(greenQuad, surface, 32, 32);

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
