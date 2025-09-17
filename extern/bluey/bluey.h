#ifndef BLUEY_H
#define BLUEY_H

#define BLUEY_SUCCESS      1
#define BLUEY_NO_SIGNAL    0
#define BLUEY_ERROR       -1
#define BLUEY_UNREACHABLE -2

struct Bluey {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO  buff_info;
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
int    Bluey_Init(struct Bluey* bluey);                                                  /* Initialize terminal state. */
int    Bluey_Deinit(struct Bluey* bluey);                                                /* Restore terminal defaults. */
void   Bluey_Enter_Alternate_Screen(void);
void   Bluey_Leave_Alternate_Screen(void);
void   Bluey_Hide_Cursor(void);
void   Bluey_Show_Cursor(void);
int    Bluey_Read_Standard_Input_Character(struct Bluey* bluey, unsigned char* out_char); /* Non-blocking function that retrieves the next character in the standard input buffer. */
int    Bluey_Flush_Standard_Input(struct Bluey* bluey);                                   /* Flush unread input. */
size_t Bluey_Write_Frame(char* frame_buf, size_t buf_siz, size_t buf_len);                /* Write a rendered frame to the terminal */
int    Bluey_Sleep_Milliseconds(unsigned int millis);                                     /* Portable sleep in milliseconds */
#endif
