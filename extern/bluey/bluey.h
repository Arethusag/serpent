#ifndef BLUEY_H
#define BLUEY_H

#include <stddef.h>

#define BLUEY_SUCCESS      1
#define BLUEY_NO_SIGNAL    0
#define BLUEY_ERROR       -1
#define BLUEY_UNREACHABLE -2

struct Bluey;
int    Bluey_Init(struct Bluey** bluey);                                                
int    Bluey_Deinit(struct Bluey* bluey);                                               
void   Bluey_Enter_Alternate_Screen(void);
void   Bluey_Leave_Alternate_Screen(void);
void   Bluey_Hide_Cursor(void);
void   Bluey_Show_Cursor(void);
int    Bluey_Read_Standard_Input_Character(struct Bluey* bluey, unsigned char* out_char);
int    Bluey_Flush_Standard_Input(struct Bluey* bluey);               
size_t Bluey_Write_Frame(char* frame_buf, size_t buf_siz, size_t buf_len);
int    Bluey_Sleep_Milliseconds(unsigned int millis);
int    Bluey_Get_Console_Dimensions(struct Bluey* bluey, unsigned int* row_count, unsigned int* col_count);

#endif
