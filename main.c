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
	unsigned int cols;
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

void set_console_dimensions(unsigned int *rows, unsigned int *cols) {
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO console_screen_buffer_info;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &console_screen_buffer_info);
	*cols = (unsigned int)(console_screen_buffer_info.srWindow.Right - console_screen_buffer_info.srWindow.Left + 1);
	*rows = (unsigned int)(console_screen_buffer_info.srWindow.Bottom - console_screen_buffer_info.srWindow.Top + 1);
#else
	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
	*rows = (unsigned int)size.ws_row;
	*cols = (unsigned int)size.ws_col;
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

Cell *allocate_console_grid(unsigned int rows, unsigned int cols) {
    unsigned int n_cells = rows * cols;
	return malloc(n_cells * sizeof(Cell));
}

void initialize_console_grid(unsigned int rows, unsigned int cols, Cell *grid) {
    unsigned int r, c;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            grid[r * cols + c] = EMPTY;
        }
    }
}

void update_console_border(unsigned int rows, unsigned int cols, Cell *grid) {
    unsigned int r, c;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < cols; c++) {
            if (r == 0 || c == 0 || r == rows - 1 || c == cols - 1) {
                grid[r * cols + c] = BLOCK;
            }
        }
    }
}

int main() {

    /* Declarations */
	GameState game_state;
	ConsoleState console_state;
    unsigned int interrupt;
    Cell* grid;

	/* Initialize Game State */
	game_state.snake_tail_length = 1;
	
	/* Get initial console dimensions */
	set_console_dimensions(&console_state.rows, &console_state.cols);

	/* Console enablement functions */
	enable_utf8_console();
	enable_virtual_terminal_processing();

    /* Enter alternate screen and hide cursor */
    fputs("\x1b[?1049h\x1b[?25l", stdout);

    /* Setup console buffer */
    grid = allocate_console_grid(console_state.rows, console_state.cols);
    initialize_console_grid(console_state.rows, console_state.cols, grid);
    update_console_border(console_state.rows, console_state.cols, grid);
    

    interrupt = 0; /*TODO: implement SIGTERM handler*/
    while (!interrupt) {
        unsigned int r,c;
        /* Move to top-left (home) and redraw the frame */
        fputs("\x1b[H", stdout);
		for (r = 0; r < console_state.rows; r++) {
            for (c = 0; c < console_state.cols; c++) {
                switch (grid[r * console_state.cols + c]) {
                    case BLOCK:
                        fputs("\xE2\x96\x88", stdout);
                    break;
                    case EMPTY:
                        fputc(' ', stdout);
                    break;
                    case SNAKE:
                        fputs("\xE2\x96\x88", stdout);
                    break;
                    case APPLE:
                        fputs("\xE2\x96\x88", stdout);
                    break;
                        
                }
            }
            if (r + 1u < console_state.rows) {
                fputc('\n', stdout);
            }
		}
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
