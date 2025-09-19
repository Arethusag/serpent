#ifndef BLUEY_INTERNAL_H
#define BLUEY_INTERNAL_H

#include "bluey.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <termios.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
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

int  Report_Last_Error(char* func_name);         
void Get_Console_Dimensions(struct Bluey* bluey);
int  Write_Standard_Output(char* output_str);
int  Flush_Standard_Output(void);

#ifdef _WIN32
int  Get_Output_Handle(HANDLE* out_handle);
int  Get_Input_Handle(HANDLE* in_handle);
int  Get_Console_Mode(HANDLE handle, DWORD* con_mode);
int  Get_Console_Screen_Buffer_Information(HANDLE handle, CONSOLE_SCREEN_BUFFER_INFO* con_buf_info);
void Set_Console_Mode(HANDLE handle, DWORD* con_mode);
int  Enable_UTF8_Console(void);
int  Enable_Virtual_Terminal_Processing(HANDLE handle, DWORD* mode);
int  Enable_Virtual_Terminal_Input(HANDLE handle, DWORD* mode);
#else
int Get_Termios(struct termios* termios);
int Set_Termios(const struct termios* termios);
int Enable_Raw_Mode(struct termios *termios);
int Get_Standard_Input_File_Descriptor_Flags(int* file_desc_flags);
int Set_Standard_Input_File_Descriptor_Flags(int* file_desc_flags);
int Enable_Standard_Input_Non_Blocking_Mode(int* file_desc_flags);
int Get_Window_Size(struct winsize* win_size);
#endif

#endif
