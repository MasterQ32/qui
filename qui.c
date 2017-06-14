#include <video/video.h>
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

driver_context_t* graphics;
video_bitmap_t *frontbuffer;

void svcWindowCreate(pid_t client, uint32_t correlation_id, size_t size, void *args)
{
	printf("Service got request!\n");
	rpc_send_dword_response(client, correlation_id, 42);
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
	
	SDL_Surface * window = SDL_SetVideoMode(1024, 768, 24, 0);
	if(window == NULL) {
		printf("Failed to open video: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}
	
	SDL_Surface * skin = IMG_Load("skin.png");
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
	
	register_message_handler("WNCREATE", svcWindowCreate);
	
	SDL_Event e;
	bool quit = false;
	SDL_Rect target = {
		16, 16,
		240, 320
	};
	int mouseX = 0, mouseY = 0;
	bool dragging = false;
	while(!quit)
	{
		while(SDL_PollEvent(&e))
		{
			if(e.type == SDL_QUIT) quit = true;
			if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE) quit = true;
			
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
		}
		
		// Clear screen
		SDL_FillRect(window, NULL, SDL_MapRGB(window->format, 0, 0x80, 0x80));
		
		// Draw window!
		{
			SDL_Rect rect, src;
			{
				target.x -= 1;
				target.y -= 1;
				target.w += 2;
				target.h += 2;
				SDL_FillRect(window, &target, SDL_MapRGB(window->format, 0xFF, 0x00, 0x00));

				target.x += 1;
				target.y += 1;
				target.w -= 2;
				target.h -= 2;
			}
			SDL_FillRect(window, &target, SDL_MapRGB(window->format, 0xC0, 0xC0, 0xC0));
			
			src = (SDL_Rect) {
				0, 0,
				4, 22
			};
			rect = (SDL_Rect) {
				target.x, target.y,
				4, 22
			};
			SDL_BlitSurface(skin, &src, window, &rect);
			
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
				SDL_BlitSurface(skin, &src, window, &rect);
				rect.y = target.y + target.h - 3;
				src.y = 41;
				SDL_BlitSurface(skin, &src, window, &rect);
			}
			
			src = (SDL_Rect) {
				34, 0,
				22, 22
			};
			rect = (SDL_Rect) {
				target.x + target.w - 22, target.y,
				4, 22
			};
			SDL_BlitSurface(skin, &src, window, &rect);
			
			src = (SDL_Rect) {
				0, 41,
				4, 3
			};
			rect = (SDL_Rect) {
				target.x, target.y + target.h - 3,
				4, 3
			};
			SDL_BlitSurface(skin, &src, window, &rect);
			
			src = (SDL_Rect) {
				34, 41,
				22, 3
			};
			rect = (SDL_Rect) {
				target.x + target.w - 22, target.y + target.h - 3,
				4, 3
			};
			SDL_BlitSurface(skin, &src, window, &rect);
			
			
			
			
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
				SDL_BlitSurface(skin, &src, window, &rect);
				src.x = 53;
				rect.x = target.x + target.w - 3;
				SDL_BlitSurface(skin, &src, window, &rect);
			}
		}
		
		SDL_BlitSurface(
			cursor,
			NULL,
			window,
			&((SDL_Rect){
				mouseX, mouseY,
				16, 24
			}));
		
		// Render everything
		SDL_Flip(window);
		SDL_Delay(10);
	}
	
	SDL_Quit();
	
	return EXIT_SUCCESS;
}