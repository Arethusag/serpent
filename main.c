#include <stdio.h>

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
	/* Nothing to do on POSIX; virtual terminal sequences are native */
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

void calculate_screen_buffer(unsigned int *rows, unsigned int *cols) {
	void* buffer_pointer = malloc();
}

int main() {

    /* Declarations */
	GameState game_state;
	ConsoleState console_state;
    char* full_block;
    unsigned int interrupt;


	/* Initialize Game State */
	game_state.snake_tail_length = 1;
	
	/* Get initial console dimensions */
	set_console_dimensions(&console_state.rows, &console_state.columns);
    /* printf("Console rows: %d\n",console_state.rows); */
    /* printf("Console columns: %d\n",console_state.columns); */
	

	/* Console enablement functions */
	enable_utf8_console();
	enable_virtual_terminal_processing();

    /* Render loop */
	full_block = "\xE2\x96\x88";
    interrupt = 0; /*TODO: implement SIGTERM handler*/

	/* Clear screen */
	puts("\033[2J\033[H");

	/* Disable cursor */
	puts("\033[?25l");

    while (!interrupt) {
        unsigned int i;
		puts("\033[H\033[2K");

		/* TODO: implement screen buffer array */
		for (i = 0; i < console_state.columns; i++) {
			printf("%s", full_block);
		}
        sleep_milliseconds(100);
    }

    return 0;
}
