#include <video/video.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <syscall.h>
#include <types.h>
#include <unistd.h>

#include <SDL/SDL.h>

driver_context_t* graphics;
video_bitmap_t *frontbuffer;

void video_init();

int main(int argc, char** argv)
{
	// Write to serial out instead of vconsole
	stdout = NULL;
	
	/*if(SDL_Init(SDL_INIT_EVERYTHING) < 0) {*/
		/*printf("Failed to initialize SDL: %s\n", SDL_GetError());*/
	/*}*/
	
	video_init();

	while (1) {
		// CLEAR
		libvideo_change_color(0,0,0,0);
		libvideo_draw_rectangle(0,0,1024,768);

		// RENDER
		for(int x = 0; x < 256; x++)
		{
			libvideo_change_color(0, x, 0, 0);
			libvideo_draw_rectangle(x,0,1,768);
			
			libvideo_change_color(0, 255, x, 0);
			libvideo_draw_rectangle(256+x,0,1,768);
			libvideo_change_color(0, 255, 255, x);
			libvideo_draw_rectangle(512+x,0,1,768);
		}
		
		// SWAP
		libvideo_do_command_buffer();
		libvideo_flip_buffers(frontbuffer);
	}
	return 0;
}

void video_init()
{
	graphics = libvideo_create_driver_context("vesa");
	if (graphics == NULL)
	{
			fprintf(stderr, "Failed to create driver context!\n");
			exit(EXIT_FAILURE);
	}
	libvideo_use_driver_context(graphics);
	libvideo_get_command_buffer(1024);

	libvideo_change_device(0);
	
	// Somehow this is necessary before setting video mode!
	libvideo_do_command_buffer();

	if (libvideo_change_display_resolution(0, 1024, 768, 24))
	{
			printf("Failed to set video mode!\n");
			exit(EXIT_FAILURE);
	}

	frontbuffer = libvideo_get_frontbuffer_bitmap(0);
	libvideo_request_backbuffers(frontbuffer, 2);
	libvideo_change_target(frontbuffer);
}