#ifndef _GPM_MSG_H
#define _GPM_MSG_H

#define MAX_MSG_STRING_SIZE    128

typedef enum MSG_TYPE{
    MSG_DUMMY   =   0      ,
    MSG_XY                 ,
    MSG_START              ,
    MSG_STOP               ,
    MSG_PAUSE              ,
    MSG_STRING 
}Msg_type;

typedef struct Wg_point{
    wg_float x;
    wg_float y;
}Wg_point;

typedef struct Wg_message{
    Msg_type type;
    union{
        wg_char string[MAX_MSG_STRING_SIZE];
        Wg_point point;
    }value;
    List_head list;
}Wg_message;

#endif

