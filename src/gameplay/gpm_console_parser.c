#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_linked_list.h>
#include <wg_iterator.h>

#include "include/gpm_console_parser.h"

#define DQUOTE_CHAR     '\"'
#define ESC_CHAR        '\\'
#define END_STREAM      '\0'

WG_PRIVATE wg_status
parse(wg_char **pos, List_head *head, SCOPE scope);

WG_PRIVATE wg_status
allocate_token(Token **token);

WG_PRIVATE wg_status
release_token(Token **token);

WG_PRIVATE wg_status
fill_token(Token *token, TOKEN_TYPE type, wg_char *start, wg_char *end);

wg_status
gpm_console_parse(wg_char *pos, List_head *head)
{
    wg_char *text = NULL;

    CHECK_FOR_NULL(pos);

    text = pos;

    return  parse(&text, head, 0);
}

WG_PRIVATE wg_status
parse(wg_char **pos, List_head *head, SCOPE scope)
{
    wg_char *start = NULL;
    wg_status status = WG_FAILURE;
    wg_char *text;
    wg_boolean end_of_stream = WG_NO;
    Token *token = NULL;

    CHECK_FOR_NULL(pos);
    CHECK_FOR_NULL(head);

    start = text = *pos;

    while (!end_of_stream){
        switch (scope){
            case IN_STRING:
                switch (*text){
                    case DQUOTE_CHAR:
                        status = allocate_token(&token);
                        CHECK_FOR_FAILURE(status);

                        fill_token(token, TOK_STRING, start, text);
                        if (WG_FAILURE == status){
                            release_token(&token);
                            return WG_FAILURE;
                        }

                        dlist_add(head, &token->head);

                        /* skip closing quoute */
                        *pos = token->end + 1;

                        return WG_SUCCESS;
                        break;
                    case ESC_CHAR:
                        if (*(text + 1) == DQUOTE_CHAR){
                            ++text;
                        }
                        break;
                    case END_STREAM:
                        return WG_FAILURE;
                    default:
                        break;
                }
                ++text;
                break;
            case IN_DEFAULT:
                if (isdigit(*text)){
                    status = parse(&text, head, IN_INT);
                } else if (*text == DQUOTE_CHAR){
                    ++text;
                    status = parse(&text, head, IN_STRING);
                } else if (isalpha(*text) || (*text == '_')){
                    status = parse(&text, head, IN_IDENTIFIER);
                } else if (*text == END_STREAM){
                    status = parse(&text, head, IN_END_STREAM);
                    end_of_stream = WG_YES;
                } else if (isblank(*text)){
                    status = parse(&text, head, IN_BLANK);
                } else {
                    status = parse(&text, head, IN_UNKNOWN);
                }

                if (WG_FAILURE == status){
                    return WG_FAILURE;
                }

                break;
            case IN_INT:
                if (!isdigit(*text)){
                    status = allocate_token(&token);
                    CHECK_FOR_FAILURE(status);

                    fill_token(token, TOK_INT, start, text);
                    if (WG_FAILURE == status){
                        release_token(&token);
                        return WG_FAILURE;
                    }

                    dlist_add(head, &token->head);
                    *pos = token->end;

                    return WG_SUCCESS;
                }
                ++text;
                break;
            case IN_IDENTIFIER:
                if (!isalpha(*text) && !isdigit(*text) && (*text != '_')){
                    status = allocate_token(&token);
                    CHECK_FOR_FAILURE(status);

                    fill_token(token, TOK_IDENTIFIER, start, text);
                    if (WG_FAILURE == status){
                        release_token(&token);
                        return WG_FAILURE;
                    }

                    dlist_add(head, &token->head);
                    *pos = token->end;

                    return WG_SUCCESS;
                    break;
                }
                ++text;
                break;
            case IN_UNKNOWN:
                status = allocate_token(&token);
                CHECK_FOR_FAILURE(status);

                fill_token(token, TOK_UNKNOWN, start, text + 1);
                if (WG_FAILURE == status){
                    release_token(&token);
                    return WG_FAILURE;
                }

                dlist_add(head, &token->head);
                *pos = token->end;

                return WG_SUCCESS;
            case IN_BLANK:
                if (!isblank(*text)){
                    *pos = text;
                    return WG_SUCCESS;
                }
                text++;
                break;
            case IN_END_STREAM:
                status = allocate_token(&token);
                CHECK_FOR_FAILURE(status);

                status = fill_token(token, TOK_END, start, text + 1);
                if (WG_FAILURE == status){
                    release_token(&token);
                    return WG_FAILURE;
                }

                dlist_add(head, &token->head);
                *pos = text;

                return WG_SUCCESS;
            default:
                WG_ERROR("Unknown scope\n");
                return WG_FAILURE;
                break;
        }
    }

    return WG_SUCCESS;
}

wg_status
gpm_console_remove_token_list(List_head *tok_list)
{
    Iterator itr = {0};
    Token *token = NULL;

    iterator_list_init(&itr, tok_list, GET_OFFSET(Token, head));

    while ((token = iterator_list_next(&itr)) != NULL){
        WG_FREE(token);
    }

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
fill_token(Token *token, TOKEN_TYPE type, wg_char *start, wg_char *end)
{
    wg_uint size = 0;
    wg_status status = WG_SUCCESS;

    CHECK_FOR_NULL(token);

    token->type  = type;
    token->start = start;
    token->end   = end;
    switch (type){
        case TOK_UNKNOWN:
            token->value.unknown = *start;
            break;
        case TOK_END:
            token->value.end = *start;
            break;
        case TOK_BLANK:
            token->value.character = *start;
            break;
        case TOK_INT:
            token->value.integer = atoi(start);
            break;
        case TOK_IDENTIFIER:
            token->value.identifier = token->string;
            break;
        case TOK_STRING:
            token->value.string = token->string;
            break;
        default:
            token->type = TOK_INVALID;
            token->value.string = token->string;
    }
    size = MIN(token->end - token->start, MAX_TOKEN_SIZE);
    strncpy(token->string, token->start, size);
    token->string[size] = '\0';

    return status;
}

WG_PRIVATE wg_status
release_token(Token **token){
    CHECK_FOR_NULL(token);

    WG_FREE(*token);

    return WG_SUCCESS;
}

WG_PRIVATE wg_status
allocate_token(Token **token)
{

    CHECK_FOR_NULL(token);

    Token *tok = NULL;
    tok = WG_CALLOC(1, sizeof (Token));
    CHECK_FOR_NULL(tok);

    list_init(&tok->head);

    *token = tok; 

    return WG_SUCCESS;
}
