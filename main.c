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

typedef enum {
    NORTH,
    SOUTH,
    EAST,
    WEST,
    NONE
}Direction;

typedef enum {
    BORDER,
    EMPTY,
    SNAKE,
    APPLE
}TileType;

typedef struct {
	unsigned int row;
	unsigned int col;
}Coordinate;

typedef struct {
	unsigned int row_count;
	unsigned int col_count;
}ConsoleState;

typedef struct {
    unsigned int is_game_init;
	Coordinate   head_coord;
	Coordinate   tail_coord;
    Coordinate   apple_coord;
    Direction    head_dir;
}GameState;

typedef struct {
    TileType  tile_type;
    Direction travel_dir;
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
    atexit(Disable_Raw_Mode); /* Register Disable_Raw_Mode() function to run on exit */
    raw_termios = original_termios;
    raw_termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_termios);
}
#endif

void Set_Console_Dimensions(unsigned int *row_count, unsigned int *col_count) {
#ifdef _WIN32
	CONSOLE_SCREEN_BUFFER_INFO console_screen_buffer_info;
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &console_screen_buffer_info);
	*col_count = (unsigned int)(console_screen_buffer_info.srWindow.Right - console_screen_buffer_info.srWindow.Left + 1);
	*row_count = (unsigned int)(console_screen_buffer_info.srWindow.Bottom - console_screen_buffer_info.srWindow.Top + 1);
#else
	struct winsize size;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
	*row_count = (unsigned int)size.ws_row;
	*col_count = (unsigned int)size.ws_col;
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
    int input_char;

    /* _kbhit() returns true if there are any unread characters in the keyboard input buffer in a non-blocking way */
    if (_kbhit()) {
        input_char = _getch();

        /* Arrow keys in virtual terminal mode send a three-character sequence: ESC (27, '[', and then 'A', 'B', 'C', or 'D'. */
        if (input_char == 27) {
            if (_kbhit() && _getch() == '[') {
                if (_kbhit()) {
                    input_char = _getch();
                    switch (input_char) {
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
    char input_char_seq[3];
    int return_val;

    /* Attempt to read a character. read() will return -1 if no data is available */
    return_val = read(STDIN_FILENO, &input_char_seq[0], 1);
    if (return_val <= 0) {
        return KEY_NONE;
    }

    if (input_char_seq[0] == '\x1b') {
        
        return_val = read(STDIN_FILENO, &input_char_seq[1], 1);
        if (return_val <= 0) {
            return KEY_NONE;
        }
        
        return_val = read(STDIN_FILENO, &input_char_seq[2], 1);
        if (return_val <= 0) {
            return KEY_NONE;
        }

        if (input_char_seq[1] == '[') {
            switch (input_char_seq[2]) {
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

Cell* Allocate_Console_Grid(unsigned int row_count, unsigned int col_count) {
    unsigned int number_of_cells = row_count * col_count;
	return malloc(number_of_cells * sizeof(Cell));
}

char* Allocate_Frame_Buffer(unsigned int row_count, unsigned int col_count) {
    size_t max_bytes_per_cell = 12; /* Colour (5 bytes) - UTF-8 Glyph (3 bytes) - Colour Reset (4 byte) */
    size_t newline = 1;
    size_t padding = 32;
    size_t number_of_bytes = row_count * (col_count * max_bytes_per_cell + newline) + padding;
    return malloc(number_of_bytes);
}

unsigned int Calculate_Index(unsigned int col_count, unsigned int row, unsigned int col) {
    unsigned int index = row * col_count + col;
    return index;
}

unsigned int Detect_Border(unsigned int row_count, unsigned int col_count, unsigned int row, unsigned int col) {
    if (row == 0 || col == 0 || row == row_count - 1 || col == col_count - 1) {
        return 1;
    } else {
        return 0;
    } 
}

void Initialize_Console_Grid(unsigned int row_count, unsigned int col_count, Cell *console_grid) {
    unsigned int row, col, index;
    for (row = 0; row < row_count; row++) {
        for (col = 0; col < col_count; col++) {
            index = Calculate_Index(col_count, row, col);
            console_grid[index].travel_dir = NONE;
            if (Detect_Border(row_count, col_count, row, col)) {
                console_grid[index].tile_type = BORDER;
            } else {
                console_grid[index].tile_type = EMPTY;
            }
        }
    }
}

Coordinate Calculate_Random_Coordinate(unsigned int row_count, unsigned int col_count) {
    Coordinate rand_coord;
    rand_coord.col = 1 + rand() % (col_count - 2);
    rand_coord.row = 1 + rand() % (row_count - 2);
    return rand_coord;
}

void Place_Apple(unsigned int row_count, unsigned int col_count, Cell* console_grid, Coordinate* apple_coord) {
    unsigned int index;
    *apple_coord = Calculate_Random_Coordinate(row_count, col_count);
    index = Calculate_Index(col_count, apple_coord->row, apple_coord->col);
	console_grid[index].tile_type = APPLE;
}

void Place_Snake_Start(unsigned int row_count, unsigned int col_count, Cell* console_grid, Coordinate* head_coord, Coordinate* tail_coord) {
    unsigned int index;
    Coordinate temp_coord;
    temp_coord = Calculate_Random_Coordinate(row_count, col_count);
    *head_coord = temp_coord;
    *tail_coord = temp_coord;
    index = Calculate_Index(col_count, temp_coord.row, temp_coord.col);
    console_grid[index].tile_type = SNAKE;
}

void Exit_Cleanup(void) {
    fputs("\x1b[?25h\x1b[?1049l", stdout); /* Show cursor and leave alternate screen */
	fputs("\033[2J\033[H", stdout);        /* Clear screen */
    fflush(stdout);
}
/* void Move_Snake_Tail(Direction tail_direction, unsigned int row_count, unsigned int col_count, Coord* tail_coord, TileType* console_grid) { */

/* } */

void Move_Snake_Head(Direction head_dir, unsigned int row_count, unsigned int col_count, Coordinate* head_coord, Cell* console_grid) {
    int next_index, next_row, next_col;
    switch (head_dir) {
    case NORTH:
        next_row = head_coord->row - 1;
        next_col = head_coord->col;
    break;
    case SOUTH:
        next_row = head_coord->row + 1;
        next_col = head_coord->col;
    break;
    case EAST: 
        next_row = head_coord->row;
        next_col = head_coord->col + 1;
    break;
    case WEST: 
        next_row = head_coord->row;
        next_col = head_coord->col - 1;
    break;
    }
    if (Detect_Border(row_count, col_count, next_row, next_col)) {
        Exit_Cleanup();
        exit(0);
    }
    next_index = Calculate_Index(col_count, next_row, next_col);
    head_coord->row = next_row;
    head_coord->col = next_col;
    console_grid[next_index].tile_type = SNAKE;
}

size_t Render_Frame(char* frame_buffer, unsigned int row_count, unsigned int col_count, Cell* console_grid) {
    unsigned int row, col;
    char* home = "\x1b[H"; /* Prepend home escape so each frame starts at origin (row 0, col 0) */
    size_t offset = 0;
    size_t home_length = strlen(home);
    memcpy(frame_buffer + offset, home, home_length);
    offset += home_length;
    for (row = 0; row < row_count; row++) {
        for (col = 0; col < col_count; col++) {
            char* tile;
            size_t tile_length;
            unsigned int index;
            index = Calculate_Index(col_count, row, col);
			switch (console_grid[index].tile_type) {
				case BORDER:
                    tile = "\x1b[37m\xE2\x96\x88\x1b[0m"; /* Light grey full-block */
                    tile_length = strlen(tile);
				break;
				case EMPTY:
					tile = " ";
                    tile_length = strlen(tile);
				break;
				case SNAKE:
                    tile = "\x1b[32m\xE2\x96\x88\x1b[0m"; /* Green full-block */
                    tile_length = strlen(tile);
				break;
				case APPLE:
                    tile = "\x1b[31m\xE2\x96\x88\x1b[0m"; /* Red full-block */
                    tile_length = strlen(tile);
				break;
			}
            memcpy(frame_buffer + offset, tile, tile_length);
            offset += tile_length;
		}
        if (row + 1u < row_count) { /* Append new line except on last row */
            frame_buffer[offset++] = '\n';
        }
    }
    return offset;
}

int main() {
	GameState game_state;
	ConsoleState console_state;
    unsigned int interrupt;
    Cell* console_grid;
    char* frame_buffer;
    size_t frame_buffer_length;
    unsigned int arrow_key;

    srand(time(NULL)); /* Set seed */

	/* Console setup */
	Set_Console_Dimensions(&console_state.row_count, &console_state.col_count);
#ifdef _WIN32
	Enable_UTF8_Console();
	Enable_Virtual_Terminal_Processing();
#else
    Enable_Raw_Mode();
    fcntl(STDIN_FILENO, F_SETFL, fcntl(STDIN_FILENO, F_GETFL) | O_NONBLOCK); /* Set stdin to non-blocking */
#endif
    fputs("\x1b[?1049h\x1b[?25l", stdout); /* Enter alternate screen and hide cursor */

    /* Grid Setup */
    console_grid = Allocate_Console_Grid(console_state.row_count, console_state.col_count);
    Initialize_Console_Grid(console_state.row_count, console_state.col_count, console_grid);
    Place_Apple(console_state.row_count, console_state.col_count, console_grid, &game_state.apple_coord);
    Place_Snake_Start(console_state.row_count, console_state.col_count, console_grid, &game_state.head_coord, &game_state.tail_coord);
    
    /* Frame buffer setup */
    frame_buffer = Allocate_Frame_Buffer(console_state.row_count, console_state.col_count);

    /* Game loop */
    interrupt = 0; /*TODO: implement SIGTERM handler*/
    while (!interrupt) {

        /* Register arrow key inputs */
        arrow_key = Get_Arrow_Key_Press();
        if (arrow_key != KEY_NONE) {

            /* Game initializes on first arrow key pressed */
            if (!game_state.is_game_init) {
                game_state.is_game_init = 1;
            }

            switch (arrow_key) {
            case KEY_UP:
                game_state.head_dir = NORTH;
			break;
            case KEY_DOWN:
                game_state.head_dir = SOUTH;
			break;
            case KEY_RIGHT:
                game_state.head_dir = EAST;
			break;
            case KEY_LEFT:
                game_state.head_dir = WEST;
			break;
			}
        }
        if (game_state.is_game_init) {
            Move_Snake_Head(game_state.head_dir, console_state.row_count, console_state.col_count, &game_state.head_coord, console_grid);
            /* Move_Snake_Tail(); */
        }
        
        frame_buffer_length = Render_Frame(frame_buffer, console_state.row_count, console_state.col_count, console_grid);
        fwrite(frame_buffer, 1, frame_buffer_length, stdout);
        fflush(stdout);
        Sleep_Milliseconds(120); /* 8 FPS */
    }
    Exit_Cleanup();
    return 0;
}
