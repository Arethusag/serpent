#include "bluey_internal.h"
#include "bluey.h"
#include <stdlib.h>
#include <time.h>


int Bluey_Init(struct Bluey** out_bluey) {
    struct Bluey* bluey = (struct Bluey*)malloc(sizeof(*bluey));
#ifdef _WIN32
    if (Get_Output_Handle(&bluey->out_handle)                                           != BLUEY_SUCCESS) goto fail;
    if (Get_Input_Handle(&bluey->in_handle)                                             != BLUEY_SUCCESS) goto fail;
    if (Get_Console_Mode(bluey->in_handle, &bluey->orig_in_mode)                        != BLUEY_SUCCESS) goto fail;
    if (Get_Console_Mode(bluey->out_handle, &bluey->orig_out_mode)                      != BLUEY_SUCCESS) goto fail;
    bluey->curr_in_mode = bluey->orig_in_mode;
    bluey->curr_out_mode = bluey->orig_out_mode;
    if (Enable_Virtual_Terminal_Processing(bluey->out_handle, &bluey->curr_out_mode)    != BLUEY_SUCCESS) goto fail;
    if (Enable_Virtual_Terminal_Input(bluey->in_handle, &bluey->curr_out_mode)          != BLUEY_SUCCESS) goto fail;
    if (Get_Console_Screen_Buffer_Information(bluey->out_handle, &bluey->con_buf_info)  != BLUEY_SUCCESS) goto fail;
    if (Enable_UTF8_Console()                                                           != BLUEY_SUCCESS) goto fail;
#else
    if (Get_Termios(&bluey->orig_termios)                                               != BLUEY_SUCCESS) goto fail;
    bluey->deriv_termios = bluey->orig_termios;
    if (Get_Standard_Input_File_Descriptor_Flags(&bluey->orig_in_file_desc_flags)       != BLUEY_SUCCESS) goto fail;
    bluey->deriv_in_file_desc_flags = bluey->orig_in_file_desc_flags;
    if (Get_Window_Size(&bluey->win_size)                                               != BLUEY_SUCCESS) goto fail;
    if (Enable_Standard_Input_Non_Blocking_Mode(&bluey->curr_in_file_desc_flags)        != BLUEY_SUCCESS) goto fail;
    if (Enable_Raw_Mode(&bluey->curr_termios)                                           != BLUEY_SUCCESS) goto fail;
#endif
    Get_Console_Dimensions(bluey);
    *out_bluey = bluey;
    return BLUEY_SUCCESS;
fail:
    free(bluey);
    return BLUEY_ERROR;
}

int Bluey_Deinit(struct Bluey* bluey) {
#ifdef _WIN32
    Set_Console_Mode(bluey->out_handle, &bluey->orig_out_mode);
    Set_Console_Mode(bluey->in_handle, &bluey->orig_in_mode);
#else
    Set_Termios(bluey->orig_termios);
    Set_Standard_Input_File_Descriptor_Flags(bluey->orig_in_file_desc_flags);
#endif
    free(bluey);
    return BLUEY_SUCCESS;
}

int Write_Standard_Output(char* output_str) {
    int return_val;
    return_val = fputs(output_str, stdout);
    if (return_val == EOF) {
        Report_Last_Error("fputs");
        return BLUEY_ERROR;
    } else if (return_val == 0) {
        return BLUEY_SUCCESS;
    }
    return BLUEY_UNREACHABLE; 
}

int Bluey_Flush_Standard_Output(void) {
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
    read_ok = ReadFile(bluey->in_handle, &read_buf, num_bytes_to_read, &num_bytes_read, NULL); 
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
    } else if (return_val == 0) {
        return BLUEY_SUCCESS;    
    }
#else
    int return_val;
    return_val = usleep(millis * 1000);
    if (return_val == -1) {
        Report_Last_Error("usleep");
    } else if (return_val == 0) {
        return BLUEY_SUCCESS;    
    }
#endif
    return BLUEY_UNREACHABLE;
}

void Bluey_Get_Console_Dimensions(struct Bluey* bluey, unsigned int* row_count, unsigned int* col_count) {
    *row_count = bluey->row_count;
    *col_count = bluey->col_count;
}
