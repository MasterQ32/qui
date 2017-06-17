#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syscall.h>
#include <sleep.h>

#include "qui.h"
#include "tfont.h"

void tfput(int x, int y, void *arg)
{
	bitmap_t * surface = arg;
	if(x < 0 || y < 0 || x >= surface->width || y >= surface->height)
		return;
	PIXREF(surface, x, y) = RGB(0, 0, 0);
}

struct glyph
{
	char code[128];
};

struct glyph font[256];

char const *getGlyph(int codepoint) {
	if(codepoint < 128) {
		return font[codepoint].code;
	}
	switch(codepoint)
	{
		case 215: // ×
			return "a5 M08 p4-4 M48 p-4-4";
		case 228: // ä
			return "a6 M46 p0-4 p-30 p-11 p02 p11 p20 p1-1 M18d M38d";
		case 246: // ö
			return "a6 M03 p02 p11 p20 p1-1 p0-2 p-1-1 p-20 p-11 M18d M38d";
		case 223: // ß
			return "a6 M02 P09 p11 p10 p1-1 p0-1 p-1-1 p2-2 p0-2 p-1-1 p-10";
		case 247: // ÷
			return "a6 M06 p40 M28d M24d";
		case 252: // ü
			return "a6 M06 p0-3 p1-1 p20 p11 p03 M18d M38d";
		case 8230: // …
			return "a6 M02d M22d M42d";
		case 8592: // ←
			return "a6 M06 p40 M17 p-1-1 p1-1";
		case 8593: // ↑
			return "a4 M14 P18 M07 p11 p1-1";
		case 8594: // →
			return "a6 M06 p40 M37 p1-1 p-1-1";
		case 8595: // ↓
			return "a4 M14 P18 M05 p1-1 p11";
		default:
			printf("Unicode: %d\n", codepoint);
			return NULL;
	}
}

void load(const char *fileName)
{
	FILE *f = fopen(fileName, "r");
	memset(font, 0, sizeof(struct glyph) * 256);
	while(!feof(f))
	{
		char c = fgetc(f);
		if(c == EOF) break;
		if(fgetc(f) != ':') {
			printf("invalid font file.\n");
			break;
		}
		for(int i = 0; i < 127; i++)
		{
			font[c].code[i + 0] = fgetc(f);
			font[c].code[i + 1] = 0;
			if(font[c].code[i + 0] == '\n' || font[c].code[i + 0] == EOF) {
				font[c].code[i] = 0;
				break;
			}
		}
		// printf("%c: %s\n", c, font[c].code);
	}
	fclose(f);
}

int main(int argc, char ** argv)
{
	load("/guiapps/ascii.tfn");
	tfont_setSize(12);
	tfont_setDotSize(0);
	tfont_setFont(&getGlyph);

	if(qui_open() == false) {
		printf("Failed to open gui!\n");
		exit(EXIT_FAILURE);
	}

	stdout = NULL; // Console only!

	char const * text = "Quäker würgen Meißen völlig übertrieben.\n"
	                    "f(x) = 10 × a ÷ 3\n"
						"↑ ↓ → ← '...' → '…'";

	int textwidth = tfont_measure_string(text, 0, tfNone);
	int textheight = 4 * tfont_getSize();

	window_t * window = qui_createWindow(textwidth + 16, textheight + 16, 0);
	if(window == NULL) {
		printf("Failed to create window!\n");
		exit(EXIT_FAILURE);
	}

	int anim = 0;
	bool requiresPaint = true; // Initially, draw a frame!
	bool requiresQuit = false;
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
			tfont_setPainter(&tfput, surface);

			// Clear window
			qui_clearBitmap(surface, RGBA(0, 0, 0, 0));

			int x = 8;
			int y = 8 + tfont_getSize();

			tfont_render_string(
				x, y,
				text,
				0,
				tfNone);

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
