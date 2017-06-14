#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syscall.h>
#include <sleep.h>

#include "qui.h"

int main(int argc, char ** argv)
{
	stdout = NULL; // Console only!

	while(qui_open() == false)
	{
		// Burn some cycles!
	}

	window_t * window = qui_createWindow(640, 480, 0);
	if(window == NULL) {
		printf("Failed to create window!\n");
		exit(EXIT_FAILURE);
	}

	int anim = 0;
	while(true)
	{
		// Clear window
		qui_clearWindow(window, RGBA(0, 0, 0, 0));

		// Draw some stuff here
		for(int i = 0; i < 256; i++)
		{
			for(int y = 0; y < window->height; y++) {
				window->frameBuffer[window->width * y + ((anim + i) % window->width)] = RGB(0, i, 0);
			}
		}

		// Send the graphics to the "server"
		qui_updateWindow(window);

		++anim;

		msleep(100);
	}

	qui_destroyWindow(window);

	return EXIT_SUCCESS;
}
