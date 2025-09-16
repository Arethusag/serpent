#ifndef BLUEY_H
#define BLUEY_H

#define BLUEY_SUCCESS      1
#define BLUEY_NO_SIGNAL    0
#define BLUEY_ERROR       -1
#define BLUEY_UNREACHABLE -2

#define KEY_NONE  0u
#define KEY_UP    1001u
#define KEY_DOWN  1002u
#define KEY_LEFT  1003u
#define KEY_RIGHT 1004u

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

int           Bluey_Init(struct Bluey* bluey);                                          /* Initialize console; returns handle */
int           Bluey_Deinit(struct Bluey* bluey);                                        /* Restore terminal state. */
void          Bluey_Enter_Screen(void);                                                 /* Enter alternate screen*/
void          Bluey_Leave_Screen(void);                                                 /* Leave alternate screen*/
void          Bluey_Hide_Cursor(void);                                                  /* Hide cursor */ 
void          Bluey_Show_Cursor(void);                                                  /* Show cursor */
unsigned int  Bluey_Get_Arrow_Key(struct Bluey* bluey);                                 /* None-blocking arrow key polling */
void          Bluey_Flush_Input(struct Bluey* bluey);                                   /* Flush unread input. */
char*         Bluey_Alloc_Frame_Buffer(unsigned int row_count, unsigned int col_count); /* Frame buffer allocator. */
void          Bluey_Free_Frame_Buffer(char* buf);                                       /* Free frame buffer */
size_t        Bluey_Write_Frame(struct Bluey* bluey, char* frame_buffer, size_t len);   /* Write a rendered frame to the terminal */
void          Bluey_Sleep_Millis(unsigned int millis);                                  /* Portable sleep in milliseconds */
 
#endif
