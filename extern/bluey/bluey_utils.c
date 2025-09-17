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


