int  Report_Last_Error(char* api_name);         
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
