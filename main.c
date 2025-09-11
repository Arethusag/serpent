#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>
#endif

typedef struct {
	unsigned int x;
	unsigned int y;
}UIntVector2;

typedef struct {
	unsigned int rows;
	unsigned int columns;
}ConsoleState;

typedef struct {
	unsigned int snake_tail_length;
	UIntVector2 snake_head_position;
}GameState;

typedef enum {
  BLOCK,
  EMPTY,
  SNAKE,
  APPLE
}Cell;


void enable_utf8_console(void) {
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
#else
	/* Nothing to do on POSIX; rely on terminal being UTF-8 */
#endif
}

void enable_virtual_terminal_processing(void) {
#ifdef _WIN32
	HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode;
	GetConsoleMode(hOutput, &dwMode);
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOutput, dwMode)) {
		puts("SetConsoleMode failed.");
		exit(1);
	}
#else
	/* Nothing to do on POSIX; virtual terminal sequences enabled */
#endif
}

void set_console_dimensions(unsigned int *rows, unsigned int *columns) {
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO console_screen_buffer_info;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &console_screen_buffer_info);
	*columns = (unsigned int)(console_screen_buffer_info.srWindow.Right - console_screen_buffer_info.srWindow.Left + 1);
	*rows = (unsigned int)(console_screen_buffer_info.srWindow.Bottom - console_screen_buffer_info.srWindow.Top + 1);
#else
	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
	*rows = (unsigned int)size.ws_row;
	*columns = (unsigned int)size.ws_col;
#endif
}

void sleep_milliseconds(unsigned int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#elif _POSIX_C_SOURCE >= 199309L
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#else
    usleep(milliseconds * 1000);
#endif
}

Cell* allocate_console_grid(unsigned int rows, unsigned int columns) {
    unsigned int number_of_cells = rows * columns;
	return malloc(number_of_cells * sizeof(Cell));
}

char* allocate_frame_buffer(unsigned int rows, unsigned int columns) {
    size_t per_cell_max = 3;
    size_t newline = 1;
    size_t padding = 32;

    /* Each row may have (cols * 3) bytes plus one newline (except for the last row */
    size_t number_of_bytes = rows * (columns * per_cell_max + newline) + padding;
    return malloc(number_of_bytes);
};

void initialize_console_grid(unsigned int rows, unsigned int columns, Cell *console_grid) {
    unsigned int r, c;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < columns; c++) {
            console_grid[r * columns + c] = EMPTY;
        }
    }
}

void update_console_border(unsigned int rows, unsigned int columns, Cell *console_grid) {
    unsigned int r, c;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < columns; c++) {
            if (r == 0 || c == 0 || r == rows - 1 || c == columns - 1) {
                console_grid[r * columns + c] = BLOCK;
            }
        }
    }
}

/* render_frame: renders the console_grid to frame_buffer (not NULL-terminated)
   returns the number of bytes written. */
size_t render_frame(char* frame_buffer, unsigned int rows, unsigned int columns, Cell* console_grid) {

    /* Prepend home escape so each frame starts at row 1 column 1 */
    char* home = "\x1b[H";
    size_t offset = 0;
    size_t home_length = strlen(home);
    memcpy(frame_buffer + offset, home, home_length);
    offset += home_length;

    /* Insert frame information */
    unsigned int r, c;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < columns; c++) {
            char* pixel;
            size_t pixel_length;
			switch (console_grid[r * columns + c]) {
				case BLOCK:
                    pixel = "\xE2\x96\x88";
                    pixel_length = strlen(pixel);
				break;
				case EMPTY:
					pixel = " ";
                    pixel_length = strlen(pixel);
				break;
				case SNAKE:
                    pixel = "\xE2\x96\x88";
                    pixel_length = strlen(pixel);
				break;
				case APPLE:
                    pixel = "\xE2\x96\x88";
                    pixel_length = strlen(pixel);
				break;
			}
            memcpy(frame_buffer + offset, pixel, pixel_length);
            offset += pixel_length;
		}
        if (r + 1u < rows) {
            frame_buffer[offset++] = '\n';
        }
    }
    return offset;
};

int main() {

    /* Declarations */
	GameState game_state;
	ConsoleState console_state;
    unsigned int interrupt;
    Cell* console_grid;
    char* frame_buffer;
    size_t frame_buffer_length;

	/* Initialize Game State */
	game_state.snake_tail_length = 1;
	
	/* Get initial console dimensions */
	set_console_dimensions(&console_state.rows, &console_state.columns);

	/* Console enablement functions */
	enable_utf8_console();
	enable_virtual_terminal_processing();

    /* Enter alternate screen and hide cursor */
    fputs("\x1b[?1049h\x1b[?25l", stdout);

    /* Setup console grid */
    console_grid = allocate_console_grid(console_state.rows, console_state.columns);
    initialize_console_grid(console_state.rows, console_state.columns, console_grid);
    update_console_border(console_state.rows, console_state.columns, console_grid);
    
    /* Setup frame buffer */
    frame_buffer = allocate_frame_buffer(console_state.rows, console_state.columns);

    interrupt = 0; /*TODO: implement SIGTERM handler*/
    while (!interrupt) {
        frame_buffer_length = render_frame(frame_buffer, console_state.rows, console_state.columns, console_grid);
        fwrite(frame_buffer, 1, frame_buffer_length, stdout);
        fflush(stdout);
        sleep_milliseconds(120); /* 8 FPS */
    }
    /* Show cursor and leave alternate screen */
    fputs("\x1b[?25h\x1b[?1049l", stdout);

	/* Clear screen */
	fputs("\033[2J\033[H", stdout);
    
    fflush(stdout);
    return 0;
}
