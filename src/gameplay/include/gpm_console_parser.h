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

/**
 * @brief Token structure
 */
typedef struct Token{
    TOKEN_TYPE type;                     /*!< type of the token               */
    union {
        wg_int   integer;                    /*!< int value           */
        wg_char  *string;                    /*!< string value        */
        wg_char  *identifier;                /*!< identifier value    */
        wg_char  *invalid;                   /*!< invalid value       */
        wg_char  unknown;                    /*!< unknown value       */
        wg_char  character;                  /*!< character value     */
        wg_char  blank;                      /*!< blank value         */
        wg_char  end;                        /*!< end of stream value */
    }value;                              /*!< token value                     */
    wg_char *start;                      /*!< ptr to first char of the token  */
    wg_char *end;                        /*!< ptr to last+1 char of the token */
    wg_char string[MAX_TOKEN_SIZE + 1];  /*!< string representation of token  */
    List_head head;                      /*!< list head                       */
}Token;

WG_PUBLIC wg_status
gpm_console_parse(wg_char *pos, List_head *head);

WG_PUBLIC wg_status
gpm_console_remove_token_list(List_head *tok_list);

WG_PUBLIC wg_status
gpm_console_remove_args(wg_char **arg_vector);

WG_PUBLIC wg_status
gpm_console_tokens_to_array(List_head *tok_list, wg_char ***array);

#endif
