#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syscall.h>
#include <init.h>
#include <rpc.h>
#include <lostio.h>

#include "qui.h"

#define CHR_WIDTH  7
#define CHR_HEIGHT 9

typedef struct termchar {
	wchar_t codepoint;
	uint8_t foreground : 4;
	uint8_t background : 4;
} termchar_t;

#define TERM_WIDTH 80
#define TERM_HEIGHT 25
#define TERM_LINES 120

#if TERM_LINES < TERM_HEIGHT
#error TERM_LINES must be larger or equal to TERM_HEIGHT!
#endif

static bool requiresPaint = true; // Initially, draw a frame!

struct {
	struct { int x, y; } cursor;
	termchar_t contents[TERM_LINES][TERM_WIDTH];
	int scroll; // offset from top
	bool cursorVisible;
} terminal = {
	{ 0, 0 },
	{ 0 },
    0,
    false,
};

static const color_t colorTable[16] = {
	0xFF000000,
    0xFF0000AA,
    0xFF00AA00,
    0xFF00AAAA,
    0xFFAA0000,
    0xFFAA00AA,
    0xFFAA5500,
    0xFFAAAAAA,

    0xFF555555,
    0xFF5555FF,
    0xFF55FF55,
    0xFF55FFFF,
    0xFFFF5555,
    0xFFFF55FF,
    0xFFFFFF55,
    0xFFFFFFFF,
};

bitmap_t * mapToColor(bitmap_t * src, color_t color)
{
	bitmap_t * dst = qui_createBitmap(src->width, src->height);
	for(int y = 0; y < src->height; y++) {
		for(int x = 0; x < src->width; x++) {
			color_t pix = PIXREF(src, x, y);
			if(pix & COLOR_AMASK) {
				PIXREF(dst, x, y) = color;
			} else {
				PIXREF(dst, x, y) = RGBA(0,0,0,0);
			}
		}
	}
	return dst;
}

void invalidate() {
	requiresPaint = true;
}

void clearTerm(int bg, int fg)
{
	for(int y = 0; y < TERM_LINES; y++) {
		for(int x = 0; x < TERM_WIDTH; x++) {
			terminal.contents[y][x].background = bg;
			terminal.contents[y][x].foreground = fg;
			terminal.contents[y][x].codepoint = ' ';
		}
	}
	terminal.cursor.x = 0;
	terminal.cursor.y = 0;
	invalidate();
}

void blinkCursor()
{
	terminal.cursorVisible = !terminal.cursorVisible;
	invalidate();
}

void tnewline()
{
	terminal.cursor.x = 0;
	terminal.cursor.y += 1;
	if(terminal.cursor.y >= TERM_LINES) {
		// Scroll everything upwards
		for(int i = 1; i < TERM_LINES; i++) {
			memcpy(
				terminal.contents[i - 1],
				terminal.contents[i - 0],
			    sizeof(termchar_t) * TERM_WIDTH);
		}
		for(int i = 0; i < TERM_WIDTH; i++) {
			terminal.contents[TERM_LINES - 1][i] = (termchar_t) {
				' ',
				0x07,
				0x00
			};
		}
		terminal.cursor.y = TERM_LINES - 1;
	}

	if(terminal.cursor.y < terminal.scroll) {
		terminal.scroll = terminal.cursor.y;
	}
	if(terminal.cursor.y >= (terminal.scroll + TERM_HEIGHT)) {
		terminal.scroll = terminal.cursor.y - TERM_HEIGHT + 1;
	}
}

void tputc(char c)
{
	switch(c) {
		case '\0':
			return;
		case '\r':
			terminal.cursor.x = 0;
			break;
		case '\n':
			tnewline();
			return;
		case '\b':
			if(terminal.cursor.x > 0) {
				terminal.cursor.x--;
			}
			break;
		case '\a':
			// TODO: Mach bing?!
			return;
		default:
			terminal.contents[terminal.cursor.y][terminal.cursor.x].codepoint
				= c;
			terminal.cursor.x ++;
			if(terminal.cursor.x >= TERM_WIDTH) {
				tnewline();
			}
			break;
	}
	invalidate();
}

static int clamp(int v, int min, int max)
{
	if(v < min) return min;
	if(v > max) return max;
	return v;
}

// Terminal schreibt nach myoutput, liest von myinput
lio_stream_t myinput, myoutput;

// Kindprozess schreibt nach "childoutput" und liest von "childinput"
lio_stream_t childinput, childoutput;

void termwrite(void const * buffer, size_t length)
{
	if(lio_write(myoutput, length, buffer) != length) {
		printf("Failed to write complete output!\n");
	}
}

int termread(void * buffer, size_t length)
{
	return lio_readf(myinput, 0, length, buffer, LIO_REQ_FILEPOS);
}

int main(int argc, char ** argv)
{
	if(qui_open() == false) {
		printf("Failed to open gui!\n");
		exit(EXIT_FAILURE);
	}

	bitmap_t * fontTemplate = qui_loadBitmap(QUI_RESOURCE("qterm-font.png"));
	if(fontTemplate == NULL) {
		printf("Failed to load qterm-font.png!\n");
		exit(EXIT_FAILURE);
	}

	bitmap_t * fonts[16];
	for(int i = 0; i < 16; i++) {
		fonts[i] = mapToColor(fontTemplate, colorTable[i]);
	}
	qui_destroyBitmap(fontTemplate);

	bitmap_t * backgrounds = qui_createBitmap(CHR_WIDTH * 16, CHR_HEIGHT);
	for(int i = 0; i < 16; i++) {
		for(int y = 0; y < CHR_HEIGHT; y++) {
			for(int x = 0; x < CHR_WIDTH; x++) {
				PIXREF(backgrounds, CHR_WIDTH * i + x, y) = colorTable[i];
			}
		}
	}

	window_t * window = qui_createWindow(
		TERM_WIDTH * CHR_WIDTH + 2,
		TERM_HEIGHT * CHR_HEIGHT + 2,
		0);
	if(window == NULL) {
		printf("Failed to create window!\n");
		exit(EXIT_FAILURE);
	}

	clearTerm(0x0, 0x7);

	// TODO: Was tut das ?
	timer_register(blinkCursor, 250000);

	uint64_t tick = get_tick_count();

	if(lio_pipe(&myinput, &childoutput, false) < 0) {
		printf("Failed to create pipe!\n");
	}
	if(lio_pipe(&childinput, &myoutput, false) < 0) {
		printf("Failed to create pipe!\n");
	}

	FILE * pout = stdout, * pin = stdin, * perr = stderr;

	stdout = stderr = __create_file_from_lio_stream(childoutput);
	stdin = __create_file_from_lio_stream(childinput);

	// init_execute(QUI_ROOT "bin/pingpong");
	init_execute("file:/apps/sh");

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
					if(e.key.keysym.unicode != 0) {
						// TODO: Encode unicode
						char input = e.key.keysym.unicode;

						termwrite(&input, sizeof input);
					}
					if(e.key.keysym.sym == SDLK_PAGEUP) {
						terminal.scroll -= 10;
						invalidate();
					}
					if(e.key.keysym.sym == SDLK_PAGEDOWN) {
						terminal.scroll += 10;
						invalidate();
					}
					break;
			}
		}

		if(requiresQuit) {
			break;
		}


		// Lese Daten vom "Kindprozess"!
		{
			char buffer[256];
			int len = termread(buffer, sizeof(buffer));
			for(int i = 0; i < len; i++) {
				tputc(buffer[i]);
			}
		}

		if(get_tick_count() > (tick + 250000)) {
			blinkCursor();
			tick = get_tick_count();
		}

		// Bring the terminal into a valid position!

		terminal.scroll = clamp(terminal.scroll, 0, TERM_LINES - TERM_HEIGHT);

		if(requiresPaint && !requiresQuit) {
			bitmap_t * surface = qui_getWindowSurface(window);
			qui_clearBitmap(surface, RGB(0, 0, 0)); // Nice black background

			int top = terminal.scroll;
			int bottom = terminal.scroll + TERM_HEIGHT;

			for(int y = top; y < bottom; y++) {
				for(int x = 0; x < TERM_WIDTH; x++) {
					termchar_t c = terminal.contents[y][x];

					qui_blitBitmapExt(
						backgrounds,
						CHR_WIDTH * c.background, 0,
						surface,
						1 + CHR_WIDTH * x, 1 + CHR_HEIGHT * (y - top),
						CHR_WIDTH, CHR_HEIGHT);

					// Blanks won't be drawn ;)
					if(c.codepoint != ' ') {
						if(c.codepoint > ' ' && c.codepoint < 128) {
							int srcX = CHR_WIDTH * ((c.codepoint - ' ') % 18);
							int srcY = CHR_HEIGHT * ((c.codepoint - ' ') / 18);

							qui_blitBitmapExt(
								fonts[c.foreground],
								srcX, srcY,
								surface,
								1 + CHR_WIDTH * x, 1 + CHR_HEIGHT * (y - top),
								CHR_WIDTH, CHR_HEIGHT);

						} else {
							// Unsupported character :(
						}
					}

					if(terminal.cursorVisible && terminal.cursor.x == x && terminal.cursor.y == y) {
						qui_blitBitmapExt(
							backgrounds,
							CHR_WIDTH * 0x0F, 0, // Select white cursor
							surface,
							1 + CHR_WIDTH * x, 1 + CHR_HEIGHT * (y - top),
							CHR_WIDTH, CHR_HEIGHT);
					}
				}
			}

			qui_updateWindow(window);
			requiresPaint = false;
		}

		// wait_for_rpc();
		yield();
	}

	fclose(stdin);
	fclose(stdout);
	fclose(stderr);

	stdin = pin;
	stdout = pout;
	stderr = perr;

	printf("CLIENT: Shutdown!\n");

	qui_destroyWindow(window);

	return EXIT_SUCCESS;
}
