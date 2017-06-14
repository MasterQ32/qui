#pragma once
#include <stdbool.h>

#define MSG_WINDOW_CREATE  "WNCREATE"
#define MSG_WINDOW_DESTROY "WNDESTRO"
#define MSG_WINDOW_UPDATE  "WNUPDATE"

struct wincreate_req
{
	uint32_t width, height;
} __attribute__ ((packed));

struct wincreate_resp
{
	uint32_t width, height, framebuf;
} __attribute__ ((packed));