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
    unsigned int is_interrupt;
    unsigned int is_game_init;
    unsigned int tail_length;
	Coordinate   head_coord;
	Coordinate   tail_coord;
    Coordinate   apple_coord;
    Direction    head_dir;
}GameState;

typedef struct {
    TileType  tile_type;
    Direction travel_dir;
}Cell;

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

unsigned int Detect_Border(unsigned int row_count, unsigned int col_count, Coordinate coord) {
    if (coord.row == 0 || coord.col == 0 || coord.row == row_count - 1 || coord.col == col_count - 1) {
        return 1;
    } else {
        return 0;
    } 
}

unsigned int Detect_Snake(unsigned int col_count, Coordinate coord, Cell* console_grid) {
    unsigned int index;
    index = Calculate_Index(col_count, coord.row, coord.col);
    if (console_grid[index].travel_dir != NONE) {
        return 1;
    } else {
        return 0;
    }
}

void Initialize_Console_Grid(unsigned int row_count, unsigned int col_count, Cell* console_grid) {
    unsigned int row, col, index; 
    Coordinate coord;
    for (row = 0; row < row_count; row++) {
        for (col = 0; col < col_count; col++) {
            index = Calculate_Index(col_count, row, col);
            console_grid[index].travel_dir = NONE;
            coord.row = row;
            coord.col = col;
            if (Detect_Border(row_count, col_count, coord)) {
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

void Place_Apple(unsigned int row_count, unsigned int col_count, Coordinate* apple_coord, Cell* console_grid) {
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

void Exit_Cleanup(unsigned int score) {
    fputs("\x1b[?25h\x1b[?1049l", stdout);
	fputs("\033[2J\033[H", stdout);       
    printf("Final Score: %d\n", score);
    fflush(stdout);
}

void Move_Snake_Tail(unsigned int col_count, Coordinate* tail_coord, Coordinate* head_coord, unsigned int tail_length, Cell* console_grid) {
    int current_index, next_row, next_col;
    current_index = Calculate_Index(col_count, tail_coord->row, tail_coord->col);

    switch (console_grid[current_index].travel_dir) {
    case NORTH:
        next_row = tail_coord->row - 1;
        next_col = tail_coord->col;
    break;
    case SOUTH:
        next_row = tail_coord->row + 1;
        next_col = tail_coord->col;
    break;
    case EAST: 
        next_row = tail_coord->row;
        next_col = tail_coord->col + 1;
    break;
    case WEST: 
        next_row = tail_coord->row;
        next_col = tail_coord->col - 1;
    break;
    }

    console_grid[current_index].travel_dir = NONE;

    if (tail_coord->row == head_coord->row && tail_coord->col == head_coord->col && tail_length > 1) {
        console_grid[current_index].tile_type = SNAKE;
    } else {
        console_grid[current_index].tile_type = EMPTY;
    }

    tail_coord->row = next_row;
    tail_coord->col = next_col;
}

unsigned int Eat_Apple(unsigned int row_count, unsigned int col_count, Coordinate* apple_coord, Coordinate head_coord, Cell* console_grid) {
    if (apple_coord->row == head_coord.row && apple_coord->col == head_coord.col) {
        Place_Apple(row_count, col_count, apple_coord, console_grid);
        return 1;
    } else {
        return 0;
    }
}

void Update_Travel_Direction(Direction head_dir, unsigned int col_count, Coordinate* head_coord, Cell* console_grid) {
    unsigned int index;
    index = Calculate_Index(col_count, head_coord->row, head_coord->col);
    console_grid[index].travel_dir = head_dir;
}

void Move_Snake_Head(Direction head_dir, unsigned int col_count, Coordinate* head_coord, Cell* console_grid) {
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
    Place_Apple(console_state.row_count, console_state.col_count, &game_state.apple_coord, console_grid);
    Place_Snake_Start(console_state.row_count, console_state.col_count, console_grid, &game_state.head_coord, &game_state.tail_coord);
    
    /* Frame buffer setup */
    frame_buffer = Allocate_Frame_Buffer(console_state.row_count, console_state.col_count);

    /* Game loop */
    game_state.is_interrupt = 0;
    game_state.is_game_init = 0;
    game_state.tail_length = 0;
    while (!game_state.is_interrupt) {

        /* Register arrow key inputs */
        arrow_key = Get_Arrow_Key_Press();
        if (arrow_key != KEY_NONE) {

            /* Game initializes on first arrow key pressed */
            if (!game_state.is_game_init) {
                game_state.is_game_init = 1;
                game_state.tail_length = 1;
            }

            switch (arrow_key) {
            case KEY_UP:
                if (game_state.head_dir != SOUTH) {
                    game_state.head_dir = NORTH;
                }
			break;
            case KEY_DOWN:
                if (game_state.head_dir != NORTH) {
                    game_state.head_dir = SOUTH;
                }
			break;
            case KEY_RIGHT:
                if (game_state.head_dir != WEST) {
                    game_state.head_dir = EAST;
                }
			break;
            case KEY_LEFT:
                if (game_state.head_dir != EAST) {
                    game_state.head_dir = WEST;
                }
			break;
			}
        }
        if (game_state.is_game_init) {
            Update_Travel_Direction(game_state.head_dir, console_state.col_count, &game_state.head_coord, console_grid);
            Move_Snake_Head(game_state.head_dir, console_state.col_count, &game_state.head_coord, console_grid);
            if (!Eat_Apple(console_state.row_count, console_state.col_count, &game_state.apple_coord, game_state.head_coord, console_grid)) {
                Move_Snake_Tail(console_state.col_count, &game_state.tail_coord, &game_state.head_coord, game_state.tail_length, console_grid);
            } else {
                game_state.tail_length++;
            }
            if (Detect_Border(console_state.row_count, console_state.col_count, game_state.head_coord) || Detect_Snake(console_state.col_count, game_state.head_coord, console_grid)) {
                Exit_Cleanup(game_state.tail_length);
                exit(0);
            }
        }
        
        frame_buffer_length = Render_Frame(frame_buffer, console_state.row_count, console_state.col_count, console_grid);
        fwrite(frame_buffer, 1, frame_buffer_length, stdout);
        fflush(stdout);
        Flush_Input();

        /* frame timer */
        if (game_state.head_dir == EAST || game_state.head_dir == WEST) {
            Sleep_Milliseconds(90);
        } else {
            Sleep_Milliseconds(120);
        }
    }
    Exit_Cleanup(game_state.tail_length);
    return 0;
}
