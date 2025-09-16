#include "bluey.h"
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
void Get_Output_Handle(HANDLE* out_handle) {
    HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
    *out_handle = handle;
    if (handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "GetStdHandle(STD_OUTPUT_HANDLE) failed: %lu\n", GetLastError());
    }
}

void Get_Input_Handle(HANDLE* in_handle) {
    HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
    *in_handle = handle;
    if (handle == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "GetStdHandle(STD_INPUT_HANDLE) failed: %lu\n", GetLastError());
    }
}

void Get_Console_Mode(HANDLE* handle, DWORD* mode) {
    if (!GetConsoleMode(*handle, mode)) {
        fprintf(stderr, "GetConsoleMode failed: %lu\n", GetLastError());
    }
}

void Get_Console_Screen_Buffer_Information(HANDLE* handle, CONSOLE_SCREEN_BUFFER_INFO* con_buf_info) {
    if (GetConsoleScreenBufferInfo(bluey->out_handle, con_buf_info) == 0) {
        fprintf(stderr, "GetConsoleScreenBufferInfo failed: %lu\n", GetLastError());
    }
}

void Set_Console_Mode(HANDLE* handle, DWORD* mode) {
    if (!SetConsoleMode(*handle, *mode)) {
        fprintf(stderr, "SetConsoleMode failed: %lu\n", GetLastError());
    }
}

void Enable_UTF8_Console(void) {
    if (!SetConsoleOutputCP(CP_UTF8)) {
        fprintf(stderr, "SetConsoleOutputCP(CP_UTF8) failed: %lu\n", GetLastError());
    }
}

void Enable_Virtual_Terminal_Processing(HANDLE* handle, DWORD* mode) {
    *mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    Set_Console_Mode(handle, mode);
}

void Enable_Virtual_Terminal_Input(HANDLE* handle, DWORD* mode) {
    *mode |= ENABLE_VIRTUAL_TERMINAL_INPUT;
    Set_Console_Mode(handle, mode);
}

#else
void Get_Termios(struct termios* termios) {
    if (tcgetattr(STDIN_FILENO, termios) == -1) {
        int err = errno;
        fprintf(stderr, "tcgetattr failed: %d\n", err);
    }
}

void Set_Termios(const struct termios* termios) {
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, termios) == -1) {
        int err = errno;
        fprintf(stderr, "tcsetattr failed: %d\n", err);
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
        int err = errno;
        fprintf(stderr, "fcntl failed %d\n", err);
    }
}

void Set_Standard_Input_File_Descriptor_Flags(int* file_desc_flags) {
    if (fcntl(STDIN_FILENO, F_SETFL, file_desc_flags) == -1) {
        int err = errno;
        fprintf(stderr, "fcntl failed %d\n", err);
    }
}

void Enable_Standard_Input_Non_Blocking_Mode(int* file_desc_flags) {
    file_desc_flags |= O_NONBLOCK;
    Set_Standard_Input_File_Descriptor_Flags(file_desc_flags);
}

void Get_Window_Size(struct winsize* win_size) {
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, win_size) == -1) {
        int err = errno;
        fprintf(stderr, "ioctl failed: %d\n", err);
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
    return BLUEY_TRUE;
}

int Bluey_Deinit(struct Bluey* bluey) {
    if (!bluey) {
        return BLUEY_ERR;
    }
#ifdef _WIN32
    Set_Console_Mode(bluey->out_handle, bluey->orig_out_mode);
    Set_Console_Mode(bluey->in_handle, bluey->orig_in_mode);
#else
    Set_Termios(bluey->orig_termios);
    Set_Standard_Input_File_Descriptor_Flags(bluey->orig_in_file_desc_flags);
#endif
    return BLUEY_OK;
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
    if (!bluey || !out_char) return BLUEY_ERR;
#ifdef _WIN32
    DWORD wait_return_val;
    DWORD read_return_val;
    DWORD num_bytes_read;
    DWORD num_bytes_to_read = 1;
    DWORD wait_millis = 0 
    wait_return_val = WaitForSingleObject(bluey->in_handle, wait_millis);
    if (wait_return_val == WAIT_FAILED) {
        fprintf(stderr, "WaitForSingleObject failed: %lu\n", (GetLastError()));
        return BLUEY_ERR;
    } else if (wait_return_val == WAIT_OBJECT_0) {
        continue;
    } else {
        return BLUEY_FALSE;
    }
    if (!ReadFile(bluey->in_handle, out_char, num_bytes_to_read, &num_bytes_read)) {
        fprintf(stderr, "ReadFile failed: %lu\n", (GetLastError()));
        return BLUEY_ERR;
    } else {
        return BLUEY_OK;
    }
  }
#else
  unsigned char buf;
  ssize_t n_read = read(STDIN_FILENO, &buf, 1);
  if (n_read == 1) {
    *out_char = buf;
    return 1;
  }
  if (n_read == 0) {
    return 0;
  }
  if (errno == EAGAIN || errno == EWOULDBLOCK) {
    return 0;
  }
  return -1;
#endif
}

void Bluey_Flush_Input(struct Bluey* bluey) {
#ifdef _WIN32
    FlushConsoleInputBuffer(bluey->in_handle);
#else
    tcflush(STDIN_FILENO, TCIFLUSH);
#endif
}

char* Bluey_Allocate_Frame_Buffer(unsigned int row_count, unsigned int col_count) {

}

void Bluey_Free_Frame_Buffer(char* buf) {

}

size_t Bluey_Write_Frame(struct Bluey* bluey, char* frame_buffer, size_t len) {
}

void Bluey_Sleep_Milliseconds(unsigned int millis) {
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
