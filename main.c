#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#endif

#define KEY_NONE  0
#define KEY_UP    1001
#define KEY_DOWN  1002
#define KEY_LEFT  1003
#define KEY_RIGHT 1004

typedef struct {
	unsigned int x;
	unsigned int y;
}UIntVector2;

typedef struct {
	unsigned int rows;
	unsigned int columns;
}ConsoleState;

typedef enum {
    NORTH,
    SOUTH,
    EAST,
    WEST
}Direction;

typedef struct {
	unsigned int snake_tail_length;
    unsigned int first_key_pressed;
	UIntVector2 snake_head_coordinate;
    UIntVector2 apple_coordinate;
    Direction snake_direction;
}GameState;

typedef enum {
  BLOCK,
  EMPTY,
  SNAKE,
  APPLE
}Cell;

/* Platform specific terminal handling */
#ifdef _WIN32
void Enable_UTF8_Console(void) {
	SetConsoleOutputCP(CP_UTF8);
}

void Enable_Virtual_Terminal_Processing(void) {
	HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwMode;
	GetConsoleMode(hOutput, &dwMode);
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
}

#else
struct termios original_termios;

void Disable_Raw_Mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_termios);
}

void Enable_Raw_Mode(void) {
    struct termios raw_termios; 
    tcgetattr(STDIN_FILENO, &original_termios);
    atexit(Disable_Raw_Mode);
    raw_termios = original_termios;
    raw_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_termios);
}
#endif

void Set_Console_Dimensions(unsigned int *rows, unsigned int *columns) {
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

void Sleep_Milliseconds(unsigned int milliseconds) {
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

unsigned int Get_Arrow_Key_Press(void) {
#ifdef _WIN32
    int console_input_character;
    /* _kbhit() returns true if there are any unread characters in the keyboard input buffer in a non-blocking way */
    if (_kbhit()) {
        console_input_character = _getch();

        /* Arrow keys in virtual terminal mode send a three-character sequence: ESC (27, '[', and then 'A', 'B', 'C', or 'D'. */
        if (console_input_character == 27) {
            if (_kbhit() && _getch() == '[') {
                if (_kbhit()) {
                    console_input_character = _getch();
                    switch (console_input_character) {
						case 'A':
							return KEY_UP;
						case 'B':
							return KEY_DOWN;
						case 'C':
							return KEY_RIGHT;
						case 'D':
							return KEY_LEFT;
                        default:
                            return KEY_NONE;
                    }
                }
            }
        }
    }
    return KEY_NONE;
#else
    char input_sequence[3];
    int return_value;

    /* Attempt to read a character. read() will return -1 if no data is available */
    return_value = read(STDIN_FILENO, &input_sequence[0], 1);
    if (return_value <= 0) {
        return KEY_NONE;
    }

    if (input_sequence[0] == '\x1b') {
        
        return_value = read(STDIN_FILENO, &input_sequence[1], 1);
        if (return_value <= 0) {
            return KEY_NONE;
        }
        
        return_value = read(STDIN_FILENO, &input_sequence[2], 1);
        if (return_value <= 0) {
            return KEY_NONE;
        }

        if (input_sequence[1] == '[') {
            switch (input_sequence[2]) {
                case 'A':
                    return KEY_UP;
                case 'B':
                    return KEY_DOWN;
                case 'C':
                    return KEY_RIGHT;
                case 'D':
                    return KEY_LEFT;
                default:
                    return KEY_NONE;
            }
        }
        return KEY_NONE;
    }
    return KEY_NONE;
#endif
}

Cell* Allocate_Console_Grid(unsigned int rows, unsigned int columns) {
    unsigned int number_of_cells = rows * columns;
	return malloc(number_of_cells * sizeof(Cell));
}

char* Allocate_Frame_Buffer(unsigned int rows, unsigned int columns) {
	/* Colour (5 bytes) - UTF-8 Glyph (3 bytes) - Colour Reset (4 byte) */
    size_t max_bytes_per_cell = 12;
    size_t newline = 1;
    size_t padding = 32;

    size_t number_of_bytes = rows * (columns * max_bytes_per_cell + newline) + padding;
    return malloc(number_of_bytes);
}

void Initialize_Console_Grid(unsigned int rows, unsigned int columns, Cell *console_grid) {
    unsigned int r, c;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < columns; c++) {
            console_grid[r * columns + c] = EMPTY;
        }
    }
}

void Update_Console_Border(unsigned int rows, unsigned int columns, Cell *console_grid) {
    unsigned int r, c;
    for (r = 0; r < rows; r++) {
        for (c = 0; c < columns; c++) {
            if (r == 0 || c == 0 || r == rows - 1 || c == columns - 1) {
                console_grid[r * columns + c] = BLOCK;
            }
        }
    }
}

UIntVector2 Calculate_Random_Coordinate(unsigned int rows, unsigned int columns) {
    UIntVector2 coordinate;
    coordinate.x = 1 + rand() % (columns - 2);
    coordinate.y = 1 + rand() % (rows - 2);
    return coordinate;
}

void Place_Apple(unsigned int rows, unsigned int columns, Cell* console_grid, UIntVector2* apple_coordinate) {
    *apple_coordinate = Calculate_Random_Coordinate(rows, columns);
	console_grid[apple_coordinate->y * columns + apple_coordinate->x] = APPLE;
}

void Place_Snake_Head(unsigned int rows, unsigned int columns, Cell* console_grid, UIntVector2* snake_head_coordinate) {
    *snake_head_coordinate = Calculate_Random_Coordinate(rows, columns);
    console_grid[snake_head_coordinate->y * columns + snake_head_coordinate->x] = SNAKE;
}

void Exit_Cleanup(void) {
    /* Show cursor and leave alternate screen */
    fputs("\x1b[?25h\x1b[?1049l", stdout);

	/* Clear screen */
	fputs("\033[2J\033[H", stdout);
    
    fflush(stdout);
}

void Move_Snake_Head(Direction snake_direction, unsigned int rows, unsigned int columns, UIntVector2* snake_head_coordinate, Cell* console_grid) {
    switch (snake_direction) {
        case NORTH:
            if (snake_head_coordinate->y > 1) {
                console_grid[snake_head_coordinate->y * columns + snake_head_coordinate->x] = EMPTY;
                snake_head_coordinate->y -= 1;
                console_grid[snake_head_coordinate->y * columns + snake_head_coordinate->x] = SNAKE;
            } else {
                Exit_Cleanup();
                exit(0);
            }
        break;
        case SOUTH:
            if (snake_head_coordinate->y < (rows - 1)) {
                console_grid[snake_head_coordinate->y * columns + snake_head_coordinate->x] = EMPTY;
                snake_head_coordinate->y += 1;
                console_grid[snake_head_coordinate->y * columns + snake_head_coordinate->x] = SNAKE;
            } else {
                Exit_Cleanup();
                exit(0);
            }
        break;
        case EAST:
            if (snake_head_coordinate->x < (columns - 1)) {
                console_grid[snake_head_coordinate->y * columns + snake_head_coordinate->x] = EMPTY;
                snake_head_coordinate->x += 1;
                console_grid[snake_head_coordinate->y * columns + snake_head_coordinate->x] = SNAKE;
            } else {
                Exit_Cleanup();
                exit(0);
            }
        break;
        case WEST:
            if (snake_head_coordinate->x > 1) {
                console_grid[snake_head_coordinate->y * columns + snake_head_coordinate->x] = EMPTY;
                snake_head_coordinate->x -= 1;
                console_grid[snake_head_coordinate->y * columns + snake_head_coordinate->x] = SNAKE;
            } else {
                Exit_Cleanup();
                exit(0);
            }
        break;
    }
}

/* Renders the console_grid to frame_buffer (not NULL-terminated). Returns the number of bytes written. */
size_t Render_Frame(char* frame_buffer, unsigned int rows, unsigned int columns, Cell* console_grid) {
    unsigned int r, c;

    /* Prepend home escape so each frame starts at row 1 column 1 */
    char* home = "\x1b[H";
    size_t offset = 0;
    size_t home_length = strlen(home);
    memcpy(frame_buffer + offset, home, home_length);
    offset += home_length;

    /* Insert frame information */
    for (r = 0; r < rows; r++) {
        for (c = 0; c < columns; c++) {
            char* pixel;
            size_t pixel_length;
			switch (console_grid[r * columns + c]) {
				case BLOCK:
                    /* "\x1b[37m" = Light Grey */
                    pixel = "\x1b[37m\xE2\x96\x88\x1b[0m";
                    pixel_length = strlen(pixel);
				break;
				case EMPTY:
					pixel = " ";
                    pixel_length = strlen(pixel);
				break;
				case SNAKE:
                    /* "\x1b[32m" = Green */
                    pixel = "\x1b[32m\xE2\x96\x88\x1b[0m";
                    pixel_length = strlen(pixel);
				break;
				case APPLE:
                    /* "\x1b[31m" = Red */
                    pixel = "\x1b[31m\xE2\x96\x88\x1b[0m";
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
}

int main() {

    /* Declarations */
	GameState game_state;
	ConsoleState console_state;
    unsigned int interrupt;
    Cell* console_grid;
    char* frame_buffer;
    size_t frame_buffer_length;
    unsigned int arrow_key_pressed;

    /* Set seed */
    srand(time(NULL));

	/* Get initial console dimensions */
	Set_Console_Dimensions(&console_state.rows, &console_state.columns);

	/* Console enablement functions */
#ifdef _WIN32
	Enable_UTF8_Console();
	Enable_Virtual_Terminal_Processing();
#else
    Enable_Raw_Mode();
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK); /* Set stdin to non-blocking */
#endif
    /* Enter alternate screen and hide cursor */
    fputs("\x1b[?1049h\x1b[?25l", stdout);

    /* Setup console grid */
    console_grid = Allocate_Console_Grid(console_state.rows, console_state.columns);
    Initialize_Console_Grid(console_state.rows, console_state.columns, console_grid);
    Update_Console_Border(console_state.rows, console_state.columns, console_grid);
    Place_Apple(console_state.rows, console_state.columns, console_grid, &game_state.apple_coordinate);
    Place_Snake_Head(console_state.rows, console_state.columns, console_grid, &game_state.snake_head_coordinate);
    
    /* Setup frame buffer */
    frame_buffer = Allocate_Frame_Buffer(console_state.rows, console_state.columns);

    /* Game loop */
    interrupt = 0; /*TODO: implement SIGTERM handler*/
    while (!interrupt) {

        /* Register arrow key inputs */
        arrow_key_pressed = Get_Arrow_Key_Press();
        if (arrow_key_pressed != KEY_NONE) {

            /* Update first key pressed indicator */
            if (game_state.first_key_pressed == 0) {
                game_state.first_key_pressed = 1;
            }

            switch (arrow_key_pressed) {
            case KEY_UP:
                game_state.snake_direction = NORTH;
			break;
            case KEY_DOWN:
                game_state.snake_direction = SOUTH;
			break;
            case KEY_RIGHT:
                game_state.snake_direction = EAST;
			break;
            case KEY_LEFT:
                game_state.snake_direction = WEST;
			break;
			}
        }
        if (game_state.first_key_pressed == 1) {
            Move_Snake_Head(game_state.snake_direction, console_state.rows, console_state.columns, &game_state.snake_head_coordinate, console_grid);
        }
        
        frame_buffer_length = Render_Frame(frame_buffer, console_state.rows, console_state.columns, console_grid);
        fwrite(frame_buffer, 1, frame_buffer_length, stdout);
        fflush(stdout);
        Sleep_Milliseconds(120); /* 8 FPS */
    }
    Exit_Cleanup();
    return 0;
}
