#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
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
	columns = (unsigned int)(csbi.srWindow.Right - csbi.srWindow.Left + 1);
	rows = (unsigned int)(csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
#else
	struct winsize size;
	ioctl(STOUT_FILENO, TIOCGWINSZ, &size);
	rows = (unsigned int)(size.ws_row);
	columns = (unsigned int)(size.ws_col);
#endif
}

int main() {


	/* Initialize Game State */
	GameState game_state;
	game_state.snake_length = 1;
	
	/* Get initial console dimensions */
	ConsoleState console_state;
	set_console_dimensions(cons);
	

	/* Enable console support for UTF-8 (WIN32 Only) */
	enable_utf8_console();

	char* full_block = "\xE2\x96\x88";
	printf("%s\n", full_block);
}