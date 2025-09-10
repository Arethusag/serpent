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


static void enable_utf8_console(void) {
#ifdef _WIN32
	SetConsoleOutputCP(CP_UTF8);
#else
	/* Nothing to do on POSIX; rely on terminal being UTF-8 */
#endif
}

void set_console_dimensions(unsigned int *rows, unsigned int *columns) {
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO console_screen_buffer_info;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
	*columns = (unsigned int)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
	*rows = (unsigned int)(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
#else
	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
	*rows = (unsigned int)size.ws_row;
	*columns = (unsigned int)size.ws_col;
#endif
}

void clear_console_screen() {
#ifdef _WIN32

#else
    puts("\033[2J\033[H");
#endif
}

void sleep_milliseconds(unsigned int milliseconds) {
#ifdef _WIN32
    Sleep(milliseconds);
#else
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000L;
    nanosleep(&ts, NULL);
#endif
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
    printf("Console rows: %d\n",console_state.rows);
    printf("Console columns: %d\n",console_state.columns);
	

	/* Enable console support for UTF-8 (WIN32 Only) */
	enable_utf8_console();

    /* Render loop */
	full_block = "\xE2\x96\x88";
    interrupt = 0;
    while (!interrupt) {
        clear_console_screen();
        printf("%s\n", full_block);
        sleep_milliseconds(100);
    }

    return 0;
}
