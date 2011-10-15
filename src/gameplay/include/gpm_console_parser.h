#ifndef _GPM_CONSOLE_PARSER_H
#define _GPM_CONSOLE_PARSER_H


#define MAX_TOKEN_SIZE  80

typedef enum TOKEN_TYPE {
    TOK_INVALID     = 0     ,
    TOK_STRING              ,
    TOK_INT                 ,
    TOK_IDENTIFIER          ,
    TOK_END                 ,
    TOK_BLANK               ,
    TOK_UNKNOWN             
} TOKEN_TYPE;

typedef enum SCOPE {
    IN_DEFAULT      = 0  ,
    IN_IDENTIFIER        ,
    IN_STRING            , 
    IN_INT               ,           
    IN_END_STREAM        ,           
    IN_BLANK             ,
    IN_UNKNOWN
} SCOPE;

typedef struct Token{
    TOKEN_TYPE type;
    union {
        wg_int   integer;
        wg_char  *string;
        wg_char  *identifier;
        wg_char  *invalid;
        wg_char  unknown;
        wg_char  character;
        wg_char  blank;
        wg_char  end;
    }value;
    wg_char *start;
    wg_char *end;
    wg_char string[MAX_TOKEN_SIZE + 1]; 
    List_head head;
}Token;

WG_PUBLIC wg_status
gpm_console_parse(wg_char *pos, List_head *head);

WG_PUBLIC wg_status
gpm_console_remove_token_list(List_head *tok_list);

#endif
