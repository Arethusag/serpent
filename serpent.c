#include "bluey.h"
#include "metrify.h"

#define KEY_NONE  0
#define KEY_UP    1001
#define KEY_DOWN  1002
#define KEY_LEFT  1003
#define KEY_RIGHT 1004

enum Direction{
    NORTH,
    SOUTH,
    EAST,
    WEST,
    NONE
};

enum TileType{
    BORDER,
    EMPTY,
    SNAKE,
    APPLE
};

struct Coordinate{
	unsigned int row;
	unsigned int col;
};

struct ConsoleState{
    unsigned int row_count;
    unsigned int col_count;
};

struct GameState{
    unsigned int        is_interrupt;
    unsigned int        is_game_init;
    unsigned int        tail_length;
	struct Coordinate   head_coord;
	struct Coordinate   tail_coord;
    struct Coordinate   apple_coord;
    enum Direction      head_dir;
};

struct Cell{
    enum TileType    tile_type;
    enum Direction   travel_dir;
};

unsigned int Get_Arrow_Key_Press(struct Bluey* bluey_handle) {
    unsigned char input_char_seq[3];
    int return_val;
    return_val = Bluey_Read_Standard_Input_Character(bluey_handle, &input_char_seq[0]);
    if (return_val <= 0) {
        return KEY_NONE;
    }
    if (input_char_seq[0] == '\x1b') {
        return_val = Bluey_Read_Standard_Input_Character(bluey_handle, &input_char_seq[1]);
        if (return_val <= 0) {
            return KEY_NONE;
        }
        return_val = Bluey_Read_Standard_Input_Character(bluey_handle, &input_char_seq[2]);
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
}

struct Cell* Allocate_Console_Grid(unsigned int row_count, unsigned int col_count) {
    unsigned int number_of_cells = row_count * col_count;
	return malloc(number_of_cells * sizeof(struct Cell));
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

void Calculate_Row_And_Column_From_Index(unsigned int index, unsigned int col_count, unsigned int* out_row, unsigned int* out_col) {
    unsigned int row;
    unsigned int col;
    row = index / col_count;
    col = index % col_count;
    *out_row = row;
    *out_col = col;
}

unsigned int* Calculate_Playable_Grid(unsigned int row_count, unsigned int col_count, struct Cell* console_grid, unsigned int* out_playable_count) {
    unsigned int  playable_count;
    unsigned int* playable_grid;
    unsigned int  grid_size;
    unsigned int  grid_index; 
    unsigned int  playable_index;
    playable_count = 0;
    grid_size = row_count * col_count;
    for (grid_index = 0; grid_index < grid_size; grid_index++) {
        if (console_grid[grid_index].TileType == EMPTY) {
            playable_count++;
        }
    }
    if (playable_count == 0) {
        *out_playable_count = 0;
        return NULL;
    }
    playable_grid = malloc(playable_count * sizeof(unsigned int));
    for (grid_index = 0; grid_index < grid_size; grid_index++) {
        if (console_grid[grid_index].TileType == EMPTY) {
            playable_grid[playable_index] = grid_index;
        }
    }
    return playable_grid;
}

unsigned int Detect_Border(unsigned int row_count, unsigned int col_count, struct Coordinate coord) {
    if (coord.row == 0 || coord.col == 0 || coord.row == row_count - 1 || coord.col == col_count - 1) {
        return 1;
    } else {
        return 0;
    } 
}

unsigned int Detect_Snake(unsigned int col_count, struct Coordinate coord, struct Cell* console_grid) {
    unsigned int index;
    index = Calculate_Index(col_count, coord.row, coord.col);
    if (console_grid[index].travel_dir != NONE) {
        return 1;
    } else {
        return 0;
    }
}

void Initialize_Console_Grid(unsigned int row_count, unsigned int col_count, struct Cell* console_grid) {
    unsigned int row, col, index; 
    struct Coordinate coord;
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

struct Coordinate Calculate_Random_Coordinate(unsigned int row_count, unsigned int col_count) {
    struct Coordinate rand_coord;
    rand_coord.col = 1 + rand() % (col_count - 2);
    rand_coord.row = 1 + rand() % (row_count - 2);
    return rand_coord;
}

void Place_Apple(unsigned int row_count, unsigned int col_count, struct Coordinate* apple_coord, struct Cell* console_grid) {
    unsigned int index;
    *apple_coord = Calculate_Random_Coordinate(row_count, col_count);
    index = Calculate_Index(col_count, apple_coord->row, apple_coord->col);
	console_grid[index].tile_type = APPLE;
}

void Place_Snake_Start(unsigned int row_count, unsigned int col_count, struct Cell* console_grid, struct Coordinate* head_coord, struct Coordinate* tail_coord) {
    unsigned int index;
    struct Coordinate temp_coord;
    temp_coord = Calculate_Random_Coordinate(row_count, col_count);
    *head_coord = temp_coord;
    *tail_coord = temp_coord;
    index = Calculate_Index(col_count, temp_coord.row, temp_coord.col);
    console_grid[index].tile_type = SNAKE;
}

void Exit_Cleanup(struct Bluey* bluey, unsigned int score) {
    Bluey_Deinit(bluey);
    Bluey_Leave_Alternate_Screen();
    Bluey_Show_Cursor();
    printf("Final Score: %d\n", score);
    Bluey_Flush_Standard_Output();
}

void Move_Snake_Tail(unsigned int col_count, struct Coordinate* tail_coord, struct Coordinate* head_coord, unsigned int tail_length, struct Cell* console_grid) {
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

unsigned int Eat_Apple(unsigned int row_count, unsigned int col_count, struct Coordinate* apple_coord, struct Coordinate head_coord, struct Cell* console_grid) {
    if (apple_coord->row == head_coord.row && apple_coord->col == head_coord.col) {
        Place_Apple(row_count, col_count, apple_coord, console_grid);
        return 1;
    } else {
        return 0;
    }
}

void Update_Travel_Direction(enum Direction head_dir, unsigned int col_count, struct Coordinate* head_coord, struct Cell* console_grid) {
    unsigned int index;
    index = Calculate_Index(col_count, head_coord->row, head_coord->col);
    console_grid[index].travel_dir = head_dir;
}

void Move_Snake_Head(enum Direction head_dir, unsigned int col_count, struct Coordinate* head_coord, struct Cell* console_grid) {
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

unsigned int Render_Frame(char* frame_buffer, unsigned int row_count, unsigned int col_count, struct Cell* console_grid) {
    unsigned int row, col;
    char* home = "\x1b[H"; /* Prepend home escape so each frame starts at origin (row 0, col 0) */
    unsigned int offset = 0;
    unsigned int home_length = 3;
    memcpy(frame_buffer + offset, home, home_length);
    offset += home_length;
    for (row = 0; row < row_count; row++) {
        for (col = 0; col < col_count; col++) {
            char* tile;
            unsigned int tile_length;
            unsigned int index;
            index = Calculate_Index(col_count, row, col);
			switch (console_grid[index].tile_type) {
				case BORDER:
                    tile = "\x1b[37m\xE2\x96\x88\x1b[0m"; /* Light grey full-block */
                    tile_length = 12; /* Colour: 5 bytes, Full block: 3 bytes, Reset: 4 bytes */
				break;
				case EMPTY:
					tile = " ";
                    tile_length = 1;
				break;
				case SNAKE:
                    tile = "\x1b[32m\xE2\x96\x88\x1b[0m"; /* Green full-block */
                    tile_length = 12;
				break;
				case APPLE:
                    tile = "\x1b[31m\xE2\x96\x88\x1b[0m"; /* Red full-block */
                    tile_length = 12;
				break;
			}
            memcpy(frame_buffer + offset, tile, tile_length);
            offset += tile_length;
		}
        if (row + 1u < row_count) {
            frame_buffer[offset++] = '\n';
        }
    }
    return offset;
}

int main() {
    struct Bluey*       bluey_handle;
	struct GameState    game_state;
    struct ConsoleState console_state;
    struct Cell*        console_grid;
    char*               frame_buffer;
    unsigned int        frame_buffer_length;
    unsigned int        arrow_key;
    srand(time(NULL));
    Bluey_Init(&bluey_handle);
    Bluey_Get_Console_Dimensions(bluey_handle, &console_state.row_count, &console_state.col_count);
    console_grid = Allocate_Console_Grid(console_state.row_count, console_state.col_count);
    Initialize_Console_Grid(console_state.row_count, console_state.col_count, console_grid);
    Bluey_Enter_Alternate_Screen();
    Bluey_Hide_Cursor();
    Place_Apple(console_state.row_count, console_state.col_count, &game_state.apple_coord, console_grid);
    Place_Snake_Start(console_state.row_count, console_state.col_count, console_grid, &game_state.head_coord, &game_state.tail_coord);
    frame_buffer = Allocate_Frame_Buffer(console_state.row_count, console_state.col_count);
    game_state.is_interrupt = 0;
    game_state.is_game_init = 0;
    game_state.tail_length  = 0;
    game_state.head_dir     = NONE;
    while (!game_state.is_interrupt) {
        arrow_key = Get_Arrow_Key_Press(bluey_handle);
        if (arrow_key != KEY_NONE) {
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
                Exit_Cleanup(bluey_handle, game_state.tail_length);
                exit(0);
            }
        }
        frame_buffer_length = Render_Frame(frame_buffer, console_state.row_count, console_state.col_count, console_grid);
        Bluey_Write_Frame(frame_buffer, 1, frame_buffer_length);
        Bluey_Flush_Standard_Input(bluey_handle);
        Bluey_Flush_Standard_Output();
        Bluey_Sleep_Milliseconds(90);
    }
    return 0;
}
