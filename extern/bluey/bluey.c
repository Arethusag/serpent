#include "bluey.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#endif

struct Bluey {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO  con_buf_info;
    HANDLE                      out_handle;
    HANDLE                      in_handle;
    DWORD                       orig_in_mode;
    DWORD                       curr_in_mode;
    DWORD                       orig_out_mode;
    DWORD                       curr_out_mode;
#else
    struct termios  orig_termios;
    struct termios  deriv_termios;
    struct winsize  win_size;
    int             orig_in_file_desc_flags;
    int             deriv_in_file_desc_flags;
#endif
    unsigned int  rows;
    unsigned int  cols;
};

#ifdef _WIN32
int Get_Output_Handle(HANDLE* out_handle) {
    HANDLE handle; 
    DWORD  error_code;
    handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE) {
        error_code = GetLastError();
        fprintf(stderr, "GetStdHandle(STD_OUTPUT_HANDLE) failed: %lu\n", error_code);
        return BLUEY_ERROR;
    } else if (handle == NULL) {
        return BLUEY_ERROR;
    } else {
        *out_handle = handle;
        return BLUEY_SUCCESS;
    }
    return BLUEY_UNREACHABLE;
}

int Get_Input_Handle(HANDLE* in_handle) {
    HANDLE handle; 
    DWORD  error_code;
    handle = GetStdHandle(STD_INPUT_HANDLE);
    *in_handle = handle;
    if (handle == INVALID_HANDLE_VALUE) {
        error_code = GetLastError();
        fprintf(stderr, "GetStdHandle(STD_INPUT_HANDLE) failed: %lu\n", error_code);
        return BLUEY_ERROR;
    } else if (handle == NULL) {
        return BLUEY_ERROR;
    } else {
        *out_handle = handle;
        return BLUEY_SUCCESS;
    }
    return BLUEY_UNREACHABLE;
}

int Get_Console_Mode(HANDLE handle, DWORD* con_mode) {
    BOOL  mode_ok;
    DWORD mode;
    DWORD error_code;
    mode_ok = GetConsoleMode(handle, &mode);
    if (!mode_ok) {
        error_code = GetLastError();
        fprintf(stderr, "GetConsoleMode failed: %lu\n", error_code);
        return BLUEY_ERROR;
    } else {
        *con_mode = mode;
        return BLUEY_SUCCESS;
    } 
    return BLUEY_UNREACHABLE;
}

int Get_Console_Screen_Buffer_Information(HANDLE handle, CONSOLE_SCREEN_BUFFER_INFO* con_buf_info) {
    BOOL                       con_buf_ok;
    CONSOLE_SCREEN_BUFFER_INFO buf_info;
    DWORD                      error_code;
    con_buf_ok = GetConsoleScreenBufferInfo(handle, &buf_info);
    if (!con_buf_ok) {
        error_code = GetLastError();
        fprintf(stderr, "GetConsoleScreenBufferInfo failed: %lu\n", error_code);
        return BLUEY_ERROR;
    } else {
        *con_buf_info = buf_info;
        return BLUEY_SUCCESS;
    }
    return BLUEY_UNREACHABLE;
}

void Set_Console_Mode(HANDLE handle, DWORD* con_mode) {
    BOOL  mode_ok;
    DWORD mode;
    DWORD error_code;
    mode_ok = SetConsoleMode(handle, &mode);
    if (!mode_ok) {
        error_code = GetLastError();
        fprintf(stderr, "SetConsoleMode failed: %lu\n", error_code);
        return BLUEY_ERROR;
    } else {
        *con_mode = mode;
        return BLUEY_SUCCESS;
    }
    return BLUEY_UNREACHABLE;
}

int Enable_UTF8_Console(void) {
    BOOL  code_page_ok;
    DWORD error_code;
    code_page_ok = SetConsoleOutputCP(CP_UTF8)
    if (!code_page_ok) {
        error_code = GetLastError();
        fprintf(stderr, "SetConsoleOutputCP(CP_UTF8) failed: %lu\n", error_code);
        return BLUEY_ERROR;
    } else {
        return BLUEY_SUCCESS;
    }
    return BLUEY_UNREACHABLE;
}

int Enable_Virtual_Terminal_Processing(HANDLE handle, DWORD* mode) {
    *mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    return Set_Console_Mode(handle, mode);
}

int Enable_Virtual_Terminal_Input(HANDLE handle, DWORD* mode) {
    *mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    return Set_Console_Mode(handle, mode);
}

#else
void Get_Termios(struct termios* termios) {
    struct termios term;
    int            return_val;
    char*          error_str;
    return_val = tcgetattr(STDIN_FILENO, &term);
    if (return_val == -1) {
        error_str = strerror(errno);
        fprintf(stderr, "tcgetattr failed: %s %d\n", error_str, errno);
        return BLUEY_ERROR;
    } else if (return_val == 0
}

void Set_Termios(const struct termios* termios) {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, termios) == -1) {
        fprintf(stderr, "tcsetattr failed: %s %d\n", strerror(errno), errno);
    }
}

void Enable_Raw_Mode(struct termios *termios) {
    termios->c_lflag &= ~(ICANON | ECHO);
    Set_Termios(termios);
}

void Get_Standard_Input_File_Descriptor_Flags(int* file_desc_flags) {
    int flags = fcntl(STDIN_FILENO, F_GETFL);
    file_desc_flags = flags;
    if (flags == -1) {
        fprintf(stderr, "fcntl failed %s %d\n", strerror(errno), errno);
    }
}

void Set_Standard_Input_File_Descriptor_Flags(int* file_desc_flags) {
    if (fcntl(STDIN_FILENO, F_SETFL, file_desc_flags) == -1) {
        fprintf(stderr, "fcntl failed %s %d\n", strerror(errno), errno);
    }
}

void Enable_Standard_Input_Non_Blocking_Mode(int* file_desc_flags) {
    file_desc_flags |= O_NONBLOCK;
    Set_Standard_Input_File_Descriptor_Flags(file_desc_flags);
}

void Get_Window_Size(struct winsize* win_size) {
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, win_size) == -1) {
        fprintf(stderr, "ioctl failed: %s %d\n", strerror(errno), errno);
    }
}
#endif

void Get_Console_Dimensions(struct Bluey* bluey) {
#ifdef _WIN32
	*col_count = (unsigned int)(bluey->con_buf_info.srWindow.Right - bluey->con_buf_info.srWindow.Left + 1);
	*row_count = (unsigned int)(bluey->con_buf_info.srWindow.Bottom - bluey->con_buf_info.srWindow.Top + 1);
#else
	*row_count = (unsigned int)bluey->size.ws_row;
	*col_count = (unsigned int)bluey->size.ws_col;
#endif
}
    
int Bluey_Init(struct Bluey* bluey) {
    if (!bluey) {
        return BLUEY_ERROR;
    }
#ifdef _WIN32
    Get_Output_Handle(bluey->out_handle);
    Get_Input_Handle(bluey->in_handle);
    Get_Console_Mode(bluey->in_handle, bluey->orig_in_mode);
    Get_Console_Mode(bluey->out_handle, bluey->orig_out_mode);
    bluey->curr_in_mode = bluey->orig_in_mode;
    bluey->curr_out_mode = bluey->orig_out_mode;
    Enable_Virtual_Terminal_Processing(bluey->out_handle, bluey->curr_out_mode);
    Enable_Virtual_Terminal_Input(bluey->in_handle, bluey->curr_out_mode);
    Get_Console_Screen_Buffer_Information(bluey->out_handle, bluey->con_buf_info);
    Enable_UTF8_Console();
#else
    Get_Termios(bluey->orig_termios);
    bluey->deriv_termios = bluey->orig_termios;
    Get_Standard_Input_File_Descriptor_Flags(bluey->orig_in_file_desc_flags);
    bluey->deriv_in_file_desc_flags = bluey->orig_in_file_desc_flags;
    Get_Window_Size(bluey->win_size);
    Enable_Standard_Input_Non_Blocking_Mode(bluey->deriv_in_file_desc_flags);
    Enable_Raw_Mode(bluey->deriv_termios);
#endif
    Get_Console_Dimensions(bluey);
    return BLUEY_SUCCESS;
}

int Bluey_Deinit(struct Bluey* bluey) {
    if (!bluey) {
        return BLUEY_ERROR;
    }
#ifdef _WIN32
    Set_Console_Mode(bluey->out_handle, bluey->orig_out_mode);
    Set_Console_Mode(bluey->in_handle, bluey->orig_in_mode);
#else
    Set_Termios(bluey->orig_termios);
    Set_Standard_Input_File_Descriptor_Flags(bluey->orig_in_file_desc_flags);
#endif
    return BLUEY_SUCCESS;
}

void Bluey_Enter_Alternate_Screen(void) {
    fputs("\x1b[?1049h", stdout);
    fflush(stdout);
}

void Bluey_Leave_Alternate_Screen(void) {
    fputs("\x1b[?1049h", stdout);
    fflush(stdout);
}

void Bluey_Hide_Cursor(void) {
    fputs("\x1b[?25l", stdout);
    fflush(stdout);
}

void Bluey_Show_Cursor(void) {
    fputs("\x1b[?25h", stdout);
    fflush(stdout);
}

int Bluey_Read_Standard_Input_Character(struct Bluey* bluey, unsigned char* out_char) {
    unsigned char read_buf;
#ifdef _WIN32
    int   read_ok;
    DWORD error_code;
    DWORD wait_return_val;
    DWORD num_bytes_read;
    DWORD num_bytes_to_read;
    DWORD wait_millis;
    num_bytes_to_read = 1;
    wait_millis       = 0; 
    wait_return_val   = WaitForSingleObject(bluey->in_handle, wait_millis);
    switch (wait_return_val) {
        case WAIT_FAILED:
            error_code = GetLastError();
            fprintf(stderr, "WaitForSingleObject failed: %lu\n", error_code);
            return BLUEY_ERROR;
        case WAIT_TIMEOUT:
            return BLUEY_NO_SIGNAL;
        case WAIT_ABANDONED:
            return BLUEY_NO_SIGNAL;
        case WAIT_OBJECT_0:
            break;
        default:
            return BLUEY_UNREACHABLE;
    }
    read_ok = ReadFile(bluey->in_handle, &read_buf, num_bytes_to_read, &num_bytes_read); 
    if (!read_ok) {
        error_code = GetLastError();
        fprintf(stderr, "ReadFile failed: %lu\n", error_code);
        return BLUEY_ERROR;
    }
    switch (num_bytes_read) {       
        case 1:
            *out_char = read_buf;
            return BLUEY_SUCCESS;
        case 0:
            return BLUEY_NO_SIGNAL;
        default:
            return BLUEY_UNREACHABLE;
    }
#else
    ssize_t num_bytes_read;
    size_t  num_bytes_to_read;
    num_bytes_to_read = 1;
    num_bytes_read    = read(STDIN_FILENO, &read_buf, num_bytes_to_read);
    switch(num_bytes_read) {
        case 1:
            *out_char = read_buf;
            return BLUEY_SUCCESS;
        case 0:
            return BLUEY_NO_SIGNAL;
        case -1:
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return BLUEY_NO_SIGNAL;
            }
            fprintf(stderr, "read failed: %s %d\n", strerror(errno), errno);
            return BLUEY_ERROR;
        default:
            return BLUEY_UNREACHABLE;
    } 
#endif
    return BLUEY_UNREACHABLE;
}

int Bluey_Flush_Input(struct Bluey* bluey) {
#ifdef _WIN32
    BOOL  flush_ok;
    DWORD error_code;
    flush_ok = FlushConsoleInputBuffer(bluey->in_handle);
    if (!flush_ok) {
        error_code = GetLastError();
        fprintf(stderr, "FlushConsoleInputBuffer failed: %lu\n", error_code);
        return BLUEY_ERROR;
    } else {
        return BLUEY_SUCCESS;
    }
#else
    int flush_return_val;
    flush_return_val = tcflush(STDIN_FILENO, TCIFLUSH);
    if (flush_return_val == 0) {
        return BLUEY_SUCCESS;
    } else if (flush_return_val = -1)
        fprintf(stderr, "tcflush failed: %s %d\n", strerror(errno), errno);
        return BLUEY_ERROR;
#endif
    return BLUEY_UNREACHABLE;
}

char* Bluey_Allocate_Frame_Buffer(unsigned int row_count, unsigned int col_count) {

}

void Bluey_Free_Frame_Buffer(char* buf) {

}

size_t Bluey_Write_Frame(struct Bluey* bluey, char* frame_buffer, size_t len) {
}

void Bluey_Sleep_Milliseconds(unsigned int millis) {
#ifdef _WIN32
    Sleep(millis);
#elif _POSIX_C_SOURCE >= 199309L
    struct timespec time;
    time.tv_sec = millis / 1000;
    time.tv_nsec = (millis % 1000) * 1000000L;
    nanosleep(&time, NULL);
#else
    usleep(millis * 1000);
#endif

}
