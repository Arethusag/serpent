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
    unsigned int  row_count;
    unsigned int  col_count;
};

int Report_Last_Error(char* api_name) {
    int return_val;
#ifdef _WIN32
    DWORD last_error_code;
    DWORD format_flags;
    DWORD format_language;
    LPSTR message_string;
    DWORD message_result_ok;
    last_error_code    = GetLastError();
    format_flags       = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
    format_language    = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
    message_string     = NULL;
    message_result_ok  = FormatMessageA(format_flags, NULL, last_error_code, format_language, &message_string, 0);
    if (message_result_ok && message_string) {
        return_val = fprintf(stderr, "%s failed: %s (%lu)\n", api_name, message_string, last_error_code);
        LocalFree(msg_buf);
    } else {
        return_val = fprintf(stderr, "%s failed: (%lu)\n", api_name, last_error_code);
    }
#else
    int   error_code;
    char* error_string;
    error_code   = errno;
    error_string = strerror(errno); 
    return_val   = fprintf(stderr, "%s failed: %s (%d)\n",api_name, error_string, error_code);
#endif
    if (return_val < 0) {
        return BLUEY_ERROR;
    }
    if (fflush(stderr) == EOF) {
        return BLUEY_ERROR
    }
    return BLUEY_SUCCESS
}

#ifdef _WIN32
int Get_Output_Handle(HANDLE* out_handle) {
    HANDLE handle; 
    DWORD  error_code;
    handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (handle == INVALID_HANDLE_VALUE) {
        Report_Last_Error("GetStdHandle(STD_OUTPUT_HANDLE)");
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
        Report_Last_Error("GetStdHandle(STD_INPUT_HANDLE)");
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
        Report_Last_Error("GetConsoleMode");
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
        Report_Last_Error("GetConsoleScreenBufferInfo");
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
        Report_Last_Error("SetConsoleMode");
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
        Report_Last_Error("SetConsoleOutput(CP_UTF8)");
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
int Get_Termios(struct termios* termios) {
    int            return_val;
    struct termios term;
    return_val = tcgetattr(STDIN_FILENO, &term);
    if (return_val == -1) {
        Report_Last_Error("tcgetattr");
        return BLUEY_ERROR;
    } else if (return_val == 0) {
        *termios = term;
        return BLUEY_SUCCESS;
    } 
    return BLUEY_UNREACHABLE;
}

int Set_Termios(const struct termios* termios) {
    int            return_val;
    struct termios term;
    return_val = tcsetattr(STDIN_FILENO, TCSAFLUSH, termios);
    if (return_val == -1) {
        Report_Last_Error("tcsetattr");
        return BLUEY_ERROR;
    } else if (return_val == 0) {
        *termios = term;
        return BLUEY_SUCCESS;
    }
    return BLUEY_UNREACHALE;
}

int Enable_Raw_Mode(struct termios *termios) {
    termios->c_lflag &= ~(ICANON | ECHO);
    return Set_Termios(termios);
}

int Get_Standard_Input_File_Descriptor_Flags(int* file_desc_flags) {
    int   flags;
    flags = fcntl(STDIN_FILENO, F_GETFL);
    if (flags == -1) {
        Report_Last_Error("fntcl");
        return BLUEY_ERROR;
    } else {
        *file_desc_flags = flags;
        return BLUEY_SUCCESS;
    }
    return BLUEY_UNREACHABLE;
}

int Set_Standard_Input_File_Descriptor_Flags(int* file_desc_flags) {
    int   return_val;
    return_val = fcntl(STDIN_FILENO, F_SETFL, file_desc_flags);
    if (return_val == -1) {
        Report_Last_Error("fntcl");
        return BLUEY_ERROR;
    } else if (return_val == 0) {
        return BLUEY_SUCCESS;
    }
    return BLUEY_UNREACHABLE;
}

int Enable_Standard_Input_Non_Blocking_Mode(int* file_desc_flags) {
    file_desc_flags |= O_NONBLOCK;
    Set_Standard_Input_File_Descriptor_Flags(file_desc_flags);
}

int Get_Window_Size(struct winsize* win_size) {
    int            return_val;
    struct winsize size;
    return_val = ioctl(STDOUT_FILENO, TIOCGWINSZ, size);
    if (return_val == -1) {
        Report_Last_Error("ioctl");
        return BLUEY_ERROR;
    } else if (return_val == 0) {
        *win_size = size;
        return BLUEY_SUCCESS;
    }
    return BLUEY_UNREACHABLE;
}
#endif

void Get_Console_Dimensions(struct Bluey* bluey) {
#ifdef _WIN32
	bluey->col_count = (unsigned int)(bluey->con_buf_info.srWindow.Right - bluey->con_buf_info.srWindow.Left + 1);
	bluey->row_count = (unsigned int)(bluey->con_buf_info.srWindow.Bottom - bluey->con_buf_info.srWindow.Top + 1);
#else
	bluey->row_count = (unsigned int)bluey->size.ws_row;
	bluey->col_count = (unsigned int)bluey->size.ws_col;
#endif
}

    
int Bluey_Init(struct Bluey* bluey) {
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
#ifdef _WIN32
    Set_Console_Mode(bluey->out_handle, bluey->orig_out_mode);
    Set_Console_Mode(bluey->in_handle, bluey->orig_in_mode);
#else
    Set_Termios(bluey->orig_termios);
    Set_Standard_Input_File_Descriptor_Flags(bluey->orig_in_file_desc_flags);
#endif
    return BLUEY_SUCCESS;
}

int Write_Standard_Output(char* output_str) {
    int   return_val;
    return_val = fputs(output, stdout);
    if (return_val == EOF) {
        Report_Last_Error("fputs");
        return BLUEY_ERROR;
    } else if (return_val == 0) {
        return BLUEY_SUCCESS;
    }
    return BLUEY_UNREACHABLE; 
    }
}

int Flush_Standard_Output(void) {
    int   return_val;
    char* error_str;
    return_val = fflush(stdout);
    if (return_val == EOF) {
        Report_Last_Error("fflush");
        return BLUEY_ERROR;
    } else if (return_val == 0) {
        return BLUEY_SUCCESS;
    }
    return BLUEY_UNREACHABLE; 
}


void Bluey_Enter_Alternate_Screen(void) {
    Write_Standard_Output("\x1b[?1049h");
    Flush_Standard_Output();
}

void Bluey_Leave_Alternate_Screen(void) {
    Write_Standard_Output("\x1b[?1049l");
    Flush_Standard_Output();
}

void Bluey_Hide_Cursor(void) {
    Write_Standard_Output("\x1b[?25l");
    Flush_Standard_Output();
}

void Bluey_Show_Cursor(void) {
    Write_Standard_Output("\x1b[?25h");
    Flush_Standard_Output();
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
            Report_Last_Error("WaitForSingleObject");
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
        Report_Last_Error("ReadFile");
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
            Report_Last_Error("read");
            return BLUEY_ERROR;
        default:
            return BLUEY_UNREACHABLE;
    } 
#endif
    return BLUEY_UNREACHABLE;
}

int Bluey_Flush_Standard_Input(struct Bluey* bluey) {
#ifdef _WIN32
    BOOL  flush_ok;
    flush_ok = FlushConsoleInputBuffer(bluey->in_handle);
    if (!flush_ok) {
        Report_Last_Error("FlushConsoleInputBuffer");
        return BLUEY_ERROR;
    } else {
        return BLUEY_SUCCESS;
    }
#else
    int   flush_return_val;
    flush_return_val = tcflush(STDIN_FILENO, TCIFLUSH);
    if (flush_return_val == 0) {
        return BLUEY_SUCCESS;
    } else if (flush_return_val == -1) {
        Report_Last_Error("tcflush");
        return BLUEY_ERROR;
    }
#endif
    return BLUEY_UNREACHABLE;
}

size_t Bluey_Write_Frame(char* frame_buf, size_t buf_siz, size_t buf_len) {
    size_t num_items_written;
    num_items_written = fwrite(frame_buf, buf_siz, buf_len, stdout);
    if (num_items_written < buf_len && num_items_written != 0) {
        Report_Last_Error("fwrite");
        return BLUEY_ERROR;
    } else if (num_items_written == buf_len) {
        return BLUEY_SUCCESS;
    } else if (num_items_written == 0) {
        return BLUEY_NO_SIGNAL;
    }
    return BLUEY_UNREACHABLE;
}

int Bluey_Sleep_Milliseconds(unsigned int millis) {
#ifdef _WIN32
    Sleep(millis);
    return BLUEY_SUCCESS;
#elif _POSIX_C_SOURCE >= 199309L
    struct timespec time;
    int             return_val;
    time.tv_sec = millis / 1000;
    time.tv_nsec = (millis % 1000) * 1000000L;
    return_val = nanosleep(&time, NULL);
    if (return_val == -1) {
        Report_Last_Error("nanosleep");
    } else if (return_val == 0);
        return BLUEY_SUCCESS;    
    }
#else
    int return_val;
    return_val = usleep(millis * 1000);
    if (return_val == -1) {
        Report_Last_Error("usleep");
    } else if (return_val == 0);
        return BLUEY_SUCCESS;    
    }
#endif
    return BLUEY_UNREACHABLE;
}
