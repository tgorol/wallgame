#include <string.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_string.h>

WG_PRIVATE wg_status
count_subs(const wg_char *string, wg_char subsym, const wg_char *subsymtext,
        wg_int *short_count, wg_int *long_count, wg_int *esc_count);

WG_PRIVATE wg_status
substitute(const wg_char *string, wg_char subsym, const wg_char *subsymtext, 
        const wg_char *text, wg_char *outstr);

wg_size
wg_strlen(const wg_char *string)
{
    return strlen(string);
}

wg_status
wg_strcpy(wg_char *dest, const wg_char *src)
{
    strcpy(dest, src);

    return WG_SUCCESS;
}

wg_status
wg_strdup(wg_char *string, wg_char **copied)
{
    wg_char *copy;
    CHECK_FOR_NULL(string);

    copy = WG_MALLOC(wg_strlen(string) + 1);
    CHECK_FOR_NULL(copy);

    wg_strcpy(copy, string);    

    *copied = copy;

    return WG_SUCCESS;
}

/**
 * @brief Convert function pointer to string
 *
 * This function is needed cause ISO forbids casting function pointers
 * to void*
 *
 * @param ptr pointer
 * @param cstr buffer
 */
void
wg_fptr_2_str(fvoid ptr, wg_char *cstr)
{
    wg_int i = 0;
    wg_char *c = (char*)&ptr;

#ifdef LITTLE_ENDIAN
    for (i = sizeof (ptr) - 1; i >= 0; --i){
        char_2_hex(c[i], cstr);
        cstr += 2;
    }
#else
    for (i = 0; i < sizeof (ptr); ++i){
        char_2_hex(c[i], cstr);
        cstr += 2;
    }
#endif

    *cstr = '\0';

    return;
}

wg_status
wg_substitute(const wg_char *string, wg_char subsym, const wg_char *subsymtext, 
        const wg_char *subtext, wg_char **new_string)
{
    wg_char *new_str = NULL;
    wg_int short_subcount = 0;
    wg_int long_subcount = 0;
    wg_int escapes  = 0;
    wg_int len = 0;
    wg_status status = WG_FAILURE;

    count_subs(string, subsym, subsymtext, &short_subcount, 
            &long_subcount, &escapes);

    len = strlen(string) - escapes - 
        short_subcount * (strlen(subsymtext) + 1) -
        long_subcount * (strlen(subsymtext) + 3)  + 
        (short_subcount + long_subcount) * strlen(subtext);

    new_str = WG_CALLOC(len + 1, sizeof (wg_char));
    CHECK_FOR_NULL(new_str);

    status = substitute(string, subsym, subsymtext, subtext, new_str);
    CHECK_FOR_FAILURE(status);

    *new_string = new_str;



    return WG_SUCCESS;
}

wg_boolean
wg_strncmp(const wg_char *str1, const wg_char *str2, 
        wg_size len, const wg_char **last)
{
    wg_boolean status = WG_FALSE;

    *last = str1;

    if (0 == strncmp(str1, str2, len)){
        *last += len;
        status = WG_TRUE;
    }

    return status;
}

wg_boolean
wg_strcmpchar(const wg_char *string, wg_char c, const wg_char **last)
{
    wg_boolean status = WG_FALSE;

    *last = string;

    if (*string == c){
        ++*last;
        status = WG_TRUE;
    }

    return status;
}

WG_PRIVATE wg_status
substitute(const wg_char *string, wg_char subsym, const wg_char *subsymtext, 
        const wg_char *text, wg_char *outstr)
{
    wg_int str_text_len = 0;
    wg_int str_symtext_len = 0;
    const wg_char *last = NULL;

    CHECK_FOR_NULL(string);
    CHECK_FOR_NULL(subsymtext);
    CHECK_FOR_NULL(outstr);

    str_text_len = strlen(text);
    str_symtext_len = strlen(subsymtext);
    while (*string != '\0'){
        if (subsym == *string){
            if (wg_strcmpchar(string + 1, subsym, &last)){
                *outstr++ = subsym;
                string = last;
            }else if (wg_strcmpchar(string + 1, '{', &last)){
                if (wg_strncmp(last, subsymtext, str_symtext_len, &last)){
                    if (wg_strcmpchar(last, '}', &last)){
                        strcpy(outstr, text);
                        outstr += str_text_len;
                    }
                }
                string = last;
            }else{
                if (wg_strncmp(string + 1, subsymtext, str_symtext_len, &last)){
                    strcpy(outstr, text);
                    outstr += str_text_len;
                }
                string = last;
            }
        }else{
            *outstr++ = *string++;
        }
    }

    return WG_SUCCESS;
}
WG_PRIVATE wg_status
count_subs(const wg_char *string, wg_char subsym, const wg_char *subsymtext, 
        wg_int *short_count, wg_int *long_count, wg_int *esc_count)
{
    wg_int str_symtext_len = 0;
    const wg_char *last = NULL;

    CHECK_FOR_NULL(string);
    CHECK_FOR_NULL(subsymtext);
    CHECK_FOR_NULL(short_count);
    CHECK_FOR_NULL(long_count);

    *short_count = 0;
    *long_count = 0;
    *esc_count = 0;

    str_symtext_len = strlen(subsymtext);
    while (*string != '\0'){
        if (subsym == *string){
            if (wg_strcmpchar(string + 1, subsym, &last)){
                /* escaped subsym, ignore */
                (*esc_count)++;
                string = last;
            }else if (wg_strcmpchar(string + 1, '{', &last)){
                if (wg_strncmp(last, subsymtext, str_symtext_len, &last)){
                    if (wg_strcmpchar(last, '}', &last)){
                        (*long_count)++;
                    }
                }
                string = last;
            }else{
                if (wg_strncmp(string + 1, subsymtext, str_symtext_len, &last)){
                    (*short_count)++;
                }
                string = last;
            }
        }else{
            ++string;
        }
    }

    return WG_SUCCESS;
}

