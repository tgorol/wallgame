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

/**
 * @brief Message point structure
 */
typedef struct Wg_point{
    wg_float x;    /*!< X coordinate */
    wg_float y;    /*!< Y coordinate */
}Wg_point;

/**
 * @brief Message structure
 */
typedef struct Wg_message{
    Msg_type type;                              /*!< type of the message */
    union{
        wg_char string[MAX_MSG_STRING_SIZE];      /*!< string value  */
        Wg_point point;                           /*!< point value   */
    }value;                                     /*!< message value       */
}Wg_message;

#endif

