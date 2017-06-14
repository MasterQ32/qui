#pragma once

#include <stdbool.h>
#include <stdint.h>

#define RGB(r,g,b) ( (((r)&0xFFUL)<<0) | (((g)&0xFFUL)<<8) | (((b)&0xFFUL)<<16) | 0xFF000000UL )
#define RGBA(r,g,b,a) ( (((r)&0xFFUL)<<0) | (((g)&0xFFUL)<<8) | (((b)&0xFFUL)<<16) |  (((a)&0xFFUL)<<24) )

typedef uint32_t color_t;

typedef struct
{
	// Created by RPC
	uint32_t id;
	color_t * frameBuffer;
	uint32_t width;
	uint32_t height;
} window_t;

bool qui_open();

window_t * qui_createWindow(uint32_t width, uint32_t height, uint32_t flags);

void qui_clearWindow(window_t * window, color_t color);

void qui_updateWindow(window_t * window);

void qui_destroyWindow(window_t * window);
