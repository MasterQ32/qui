#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syscall.h>
#include <init.h>

#include "qui.h"

#define CHR_WIDTH  7
#define CHR_HEIGHT 9

int main(int argc, char ** argv)
{
	if(qui_open() == false) {
		printf("Failed to open gui!\n");
		exit(EXIT_FAILURE);
	}

	bitmap_t * font = qui_loadBitmap(QUI_RESOURCE("qterm-font.png"));
	if(font == NULL) {
		printf("Failed to load qterm-font.png!\n");
		exit(EXIT_FAILURE);
	}

	window_t * window = qui_createWindow(
		80 * CHR_WIDTH + 2,
		25 * CHR_HEIGHT + 2,
		0);
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
			}
		}

		if(requiresQuit) {
			break;
		}

		if(requiresPaint && !requiresQuit) {
			bitmap_t * surface = qui_getWindowSurface(window);
			qui_clearBitmap(surface, RGB(0, 0, 0)); // Nice black background

			// Render text content here...
			char const * str = "Hello, World!";

			for(int i = 0; str[i]; i++) {
				char c = str[i];
				if(c >= ' ' && c < 128) {
					int srcX = CHR_WIDTH * ((c - ' ') % 18);
					int srcY = CHR_HEIGHT * ((c - ' ') / 18);

					qui_blitBitmapExt(
						font,
						srcX, srcY,
						surface,
						1 + CHR_WIDTH * i, 1 + 0 * CHR_HEIGHT,
						CHR_WIDTH, CHR_HEIGHT);

				} else {
					// Unsupported character :(
				}
			}

			qui_updateWindow(window);
			requiresPaint = false;
		}

		wait_for_rpc();
	}

	printf("CLIENT: Shutdown!\n");

	qui_destroyWindow(window);

	return EXIT_SUCCESS;
}
