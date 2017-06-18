#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syscall.h>
#include <init.h>
#include <rpc.h>
#include <lostio.h>
#include <ctype.h>

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
	int fg : 4;
	int bg : 4;
} terminal = {
	{ 0, 0 },
	{ 0 },
    0,
    false,
    0x07, 0x00
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
	terminal.fg = fg;
	terminal.bg = bg;
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

#define VT100_BUFFER_SIZE 64
static struct {
	int pos;
	bool active;
	char data[VT100_BUFFER_SIZE];
} buffer = {
	0, false, { 0 },
};

struct vt100_sequence_handler {
	char const * pattern;
	void (*handler)(int nums, int n1, int n2); // bitmask with (1 | 2)
};

static void vt100_text_attr(int mask, int n1, int n2);
static void vt100_cursor_up(int mask, int n1, int n2);
static void vt100_cursor_down(int mask, int n1, int n2);
static void vt100_cursor_left(int mask, int n1, int n2);
static void vt100_cursor_right(int mask, int n1, int n2);
static void vt100_clear_screen(int mask, int n1, int n2);
static void vt100_clear_line(int mask, int n1, int n2);

static void vt100_cursor_bol_down(int mask, int n1, int n2);
static void vt100_cursor_bol_up(int mask, int n1, int n2);

static void vt100_cursor_direct(int mask, int n1, int n2);
static void vt100_cursor_save(int mask, int n1, int n2);
static void vt100_cursor_restore(int mask, int n1, int n2);

static struct vt100_sequence_handler vt100_commands[] = {
    // ANSI
    {"[\1A",        vt100_cursor_up},
    {"[\1B",        vt100_cursor_down},
    {"[\1C",        vt100_cursor_right},
    {"[\1D",        vt100_cursor_left},
    {"[\1E",        vt100_cursor_bol_down},
    {"[\1F",        vt100_cursor_bol_up},
    {"[\1;\1f",     vt100_cursor_direct},
    {"[\1;\1H",     vt100_cursor_direct},
    {"[H",          vt100_cursor_direct},
    {"[s",          vt100_cursor_save},
    {"[u",          vt100_cursor_restore},

    {"[J",          vt100_clear_screen},
    {"[\1J",        vt100_clear_screen},
    {"[K",          vt100_clear_line},
    {"[\1K",        vt100_clear_line},
	{"[\1;\1m",     vt100_text_attr},
	{"[\1m",        vt100_text_attr},
	{"[m",          vt100_text_attr},

	{ NULL, NULL },
    // VT100
    // ...
};

void rawputc(char c)
{
	switch(c) {
		case '\0':
		case '\033':
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
			terminal.contents[terminal.cursor.y][terminal.cursor.x] =
				(struct termchar) { c, terminal.fg, terminal.bg };
			terminal.cursor.x ++;
			if(terminal.cursor.x >= TERM_WIDTH) {
				tnewline();
			}
			break;
	}
	invalidate();
}

void tputc(char c)
{
	if(buffer.active) {
		// We received an escape code, now process it
		buffer.data[buffer.pos++] = c;

		if(buffer.pos >= VT100_BUFFER_SIZE) {
			// Flush!
			for(int i = 0; i < buffer.pos; i++) {
				rawputc(buffer.data[i]);
			}
			buffer.active = false;
			return;
		}

		// try pattern matching here :)
		bool allFinished = true;
		for(struct vt100_sequence_handler * it = vt100_commands; it->pattern != NULL; it++) {

			// Skip out the escape from the pattern, we already had this!
			char const * pat = it->pattern;
			char const * str = buffer.data;
			int n1 = 0, n2 = 0;
			int bitmask = 0;
			bool hasSep = 0;
			bool valid = true;

			while(*pat && *str && valid) {
				switch(*pat) {
					case '\1': { // Matches number
						if(!isdigit(*str)) {
							valid = false;
							break;
						}
						int * n = &n1;
						if(hasSep) {
							n = &n2;
							bitmask |= 2;
						} else {
							bitmask |= 1;
						}
						*n = 0;
						while(isdigit(*str)) {
							*n *= 10;
							*n += (*str++) - '0';
						}
						break;
					}
					case ';':
						if(hasSep) {
							valid = false;
						}
						hasSep = true;
						// no break here, we want to fall through!
					default: // must match
						if(*str++ != *pat) {
							valid = false;
						}
						break;
				}
				if(valid) {
					pat++;
				}
			}

			allFinished &= (*str == 0 && *pat == 0);

			if(valid && *pat == 0 && *str == 0) {
				// We done, let's get started!
				it->handler(bitmask, n1, n2);
				buffer.active = false;
				return;
			}
		}

		if(allFinished) {
			// No match found, all patterns exhausted...
			for(int i = 0; i < buffer.pos; i++) {
				rawputc(buffer.data[i]);
			}
			buffer.active = false;
			return;
		}

	} else {
		if(c == '\033') {
			// Escape code
			buffer.pos = 0;
			buffer.active = true;
			memset(buffer.data, 0, VT100_BUFFER_SIZE);
		} else {
			rawputc(c);
		}
	}
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

void termputkey(SDL_keysym key)
{

	const char* res = NULL;

	// TODO: vt52-Mode und Cursor Key Mode
	switch (key.sym) {
		case SDLK_UP:
			// vt52: <ESC>A  w CKM: \033[A  wo CKM: \033OA
			if (key.mod & KMOD_CTRL) {
				res = "\033[1;5A";
			} else {
				res = "\033[A";
			}
			break;

		case SDLK_DOWN:
			// vt52: <ESC>B  w CKM: \033[B  wo CKM: \033OB
			if (key.mod & KMOD_CTRL) {
				res = "\033[1;5B";
			} else {
				res = "\033[B";
			}
			break;

		case SDLK_RIGHT:
			// vt52: <ESC>C  w CKM: \033[C  wo CKM: \033OC
			if (key.mod & KMOD_CTRL) {
				res = "\033[1;5C";
			} else {
				res = "\033[C";
			}
			break;

		case SDLK_LEFT:
			// vt52: <ESC>D  w CKM: \033[D  wo CKM: \033OD
			if (key.mod & KMOD_CTRL) {
				res = "\033[1;5D";
			} else {
				res = "\033[D";
			}
			break;

		case SDLK_HOME:
			// TODO: Wo ist der definiert? Der originale vt100 scheint keine
			// solche Taste gehabt zu haben (Sequenz von Linux wird hier benutzt)
			res = "\033[H";
			break;

		case SDLK_END:
			// TODO: Siehe KEYCODE_HOME
			res = "\033[F";
			break;

		case SDLK_PAGEUP:
			// TODO: Siehe KEYCODE_HOME
			res = "\033[5~";
			break;

		case SDLK_PAGEDOWN:
			// TODO: Siehe KEYCODE_HOME
			res = "\033[6~";
			break;

		case SDLK_INSERT:
			// TODO: Siehe KEYCODE_HOME
			res = "\033[2~";
			break;

		case SDLK_DELETE:
			// TODO: Siehe KEYCODE_HOME
			res = "\033[3~";
			break;

		case SDLK_F1:
			res = "\033OP";
			break;

		case SDLK_F2:
			res = "\033OQ";
			break;

		case SDLK_F3:
			res = "\033OR";
			break;

		case SDLK_F4:
			res = "\033OS";
			break;

		case SDLK_F5:
			res = "\033[15~";
			break;

		case SDLK_F6:
			res = "\033[17~";
			break;

		case SDLK_F7:
			res = "\033[18~";
			break;

		case SDLK_F8:
			res = "\033[19~";
			break;

		case SDLK_F9:
			res = "\033[20~";
			break;

		case SDLK_F10:
			res = "\033[21~";
			break;

		case SDLK_F11:
			res = "\033[23~";
			break;

		case SDLK_F12:
			res = "\033[24~";
			break;

		default:
			// ASCII Input only... :(
			// TODO: utf-8 encoding here!
			if(key.unicode > 0 && key.unicode < 128) {
				char input = key.unicode;
				termwrite(&input, sizeof input);
				return;
			}
			break;
	}

	if(res != NULL) {
		int len = strlen(res);
		termwrite(res, len);
	}
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
	pid_t shellpid = init_execute("file:/apps/sh");

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
					if(e.key.keysym.mod & KMOD_LSHIFT && e.key.keysym.sym == SDLK_PAGEUP) {
						terminal.scroll -= 10;
						invalidate();
					}
					else if(e.key.keysym.mod & KMOD_LSHIFT && e.key.keysym.sym == SDLK_PAGEUP) {
						terminal.scroll += 10;
						invalidate();
					} else {
						termputkey(e.key.keysym);
					}
					break;
			}
		}

		if(get_parent_pid(shellpid) == 0) {
			requiresQuit = true;
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

	lio_close(myinput);
	lio_close(myoutput);
	lio_close(childoutput);
	lio_close(childinput);

	printf("CLIENT: Shutdown!\n");

	qui_destroyWindow(window);

	return EXIT_SUCCESS;
}

// VT100 Implementation


static void vt100_text_attr_param(int n)
{
    // Zum Umrechnen der ANSI Farbcodes in die VGA Palette.
    static char colors[8] = { 0, 4, 2, 6, 1, 5, 3, 7 };
    switch(n)
    {
        case 0: // Normal
            // vt100_reversed_colors(vterm, false);
            terminal.fg = 0x07;
			terminal.bg = 0x00;
            break;

        case 1: // Fett
            // out->current_color.bold = 1;
			terminal.fg |= 0x8;
            break;

        case 5: // Blinken
            // out->current_color.blink = 1;
            break;

        case 7: // Hintergrundfarbe auf Vordergrundfarbe
            // vt100_reversed_colors(vterm, true);
            break;

        case 30 ... 37: // Vordergrundfarbe
            terminal.fg = (terminal.fg & 0x8) | (colors[n - 30] & 0x7);
            break;

        case 40 ... 47: // Hintergrundfarbe
            terminal.bg = colors[n - 40];
            break;
    }
	invalidate();
}

static void vt100_text_attr(int nums, int n1, int n2)
{
	if(!(nums & 1)) {
	   n1 = 0;
	}
	vt100_text_attr_param(n1);

	if (nums & 2)  {
	   vt100_text_attr_param(n2);
	}
}



static void vt100_cursor_up(int mask, int n1, int n2)
{
    if (!(mask & 1)) {
        n1 = 1;
    }
	if(terminal.cursor.y >= n1) {
		terminal.cursor.y -= n1;
	}
	invalidate();
}

static void vt100_cursor_down(int mask, int n1, int n2)
{
    if (!(mask & 1)) {
        n1 = 1;
    }
	if(terminal.cursor.y < (TERM_LINES - n1)) {
		terminal.cursor.y += n1;
	}
	invalidate();
}

static void vt100_cursor_left(int mask, int n1, int n2)
{
    if (!(mask & 1)) {
        n1 = 1;
    }
	if(terminal.cursor.x >= n1) {
		terminal.cursor.x -= n1;
	}
	invalidate();
}

static void vt100_cursor_right(int mask, int n1, int n2)
{
    if (!(mask & 1)) {
        n1 = 1;
    }
	if(terminal.cursor.x < (TERM_WIDTH - n1)) {
		terminal.cursor.x += n1;
	}
	invalidate();
}


static void vt100_clear_screen(int mask, int n1, int n2)
{
	int cx = terminal.cursor.x;
	int cy = terminal.cursor.y;

	if(!(mask & 1)) { n1 = 0; }

	termchar_t clrchr = {
		' ', terminal.fg, terminal.bg,
	};

	switch(n1) {
		case 0: // ab cursor
			for(int x = cx; x < TERM_WIDTH; x++) {
				terminal.contents[cy][x] = clrchr;
			}
			for(int y = cy + 1; y < terminal.scroll + TERM_HEIGHT; y++) {
				for(int x = 0; x < TERM_WIDTH; x++) {
					terminal.contents[y][x] = clrchr;
				}
			}
			break;
		case 1: // bis cursor
			for(int y = terminal.scroll; y < cy; y++) {
				for(int x = 0; x < TERM_WIDTH; x++) {
					terminal.contents[y][x] = clrchr;
				}
			}
			for(int x = 0; x <= cx; x++) {
				terminal.contents[cy][x] = clrchr;
			}
			break;
		case 2: // ganzer bildschirm
			for(int y = 0; y < TERM_HEIGHT; y++) {
				for(int x = 0; x < TERM_WIDTH; x++) {
					terminal.contents[terminal.scroll + y][x] = clrchr;
				}
			}
			break;
	}

	terminal.cursor.x = cx;
	terminal.cursor.y = cy;
	invalidate();
}


static void vt100_clear_line(int mask, int n1, int n2)
{
	int cx = terminal.cursor.x;
	int cy = terminal.cursor.y;

	if(!(mask & 1)) { n1 = 0; }

	termchar_t clrchr = {
		' ', terminal.fg, terminal.bg,
	};

	switch(n1) {
		case 0: // ab cursor
			for(int x = cx; x < TERM_WIDTH; x++) {
				terminal.contents[cy][x] = clrchr;
			}
			break;
		case 1: // bis cursor
			for(int x = 0; x <= cx; x++) {
				terminal.contents[cy][x] = clrchr;
			}
			break;
		case 2: // ganzer bildschirm
			for(int x = 0; x < TERM_WIDTH; x++) {
				terminal.contents[cy][x] = clrchr;
			}
			break;
	}

	terminal.cursor.x = cx;
	terminal.cursor.y = cy;
	invalidate();
}

static void vt100_cursor_direct(int mask, int n1, int n2)
{
	if(!(mask & 1)) n1 = 1;
	if(!(mask & 2)) n2 = 1;

	int x = n1 - 1;
	int y = n2 - 1;
	if(x < 0) x = 0;
	if(y < 0) y = 0;
	if(x >= TERM_WIDTH) x = TERM_WIDTH - 1;
	if(y >= TERM_LINES) y = TERM_LINES - 1;

	terminal.cursor.x = x;
	terminal.cursor.y = y;
	invalidate();
}

struct {
	int x, y;
	bool valid;
} cursorSave = { 0, 0, false };

static void vt100_cursor_save(int mask, int n1, int n2)
{
	cursorSave.x = terminal.cursor.x;
	cursorSave.y = terminal.cursor.y;
	cursorSave.valid = true;
}

static void vt100_cursor_restore(int mask, int n1, int n2)
{
	if(cursorSave.valid == true) {
		terminal.cursor.x = cursorSave.x;
		terminal.cursor.y = cursorSave.y;
	}
	invalidate();
}

static void vt100_cursor_bol_up(int mask, int n1, int n2)
{
    if (!(mask & 1)) {
        n1 = 1;
    }
	if(terminal.cursor.y >= n1) {
		terminal.cursor.x = 0;
		terminal.cursor.y -= n1;
	}
	invalidate();
}

static void vt100_cursor_bol_down(int mask, int n1, int n2)
{
    if (!(mask & 1)) {
        n1 = 1;
    }
	if(terminal.cursor.y < (TERM_LINES - n1)) {
		terminal.cursor.x = 0;
		terminal.cursor.y += n1;
	}
	invalidate();
}
