#ifndef _WG_STRING_H
#define _WG_STRING_H


#define FVOID_BUFFER_SIZE ((sizeof (fvoid) * 2) + 1)

#define DECLARE_FUNC_STR_BUFFER(name) wg_char name[FVOID_BUFFER_SIZE] = {0}

WG_PUBLIC wg_size
wg_strlen(const wg_char *string);

WG_PUBLIC wg_status
wg_strcpy(wg_char *dest, const wg_char *src);

WG_PUBLIC wg_status
wg_strdup(wg_char *string, wg_char **copied);

WG_PUBLIC wg_boolean
wg_strcmpchar(const wg_char *string, wg_char c, const wg_char **last);

WG_PUBLIC wg_boolean
wg_strncmp(const wg_char *str1, const wg_char *str2,
        wg_size len, const wg_char **last);

WG_PUBLIC wg_status
wg_substitute(const wg_char *string, wg_char subsym, const wg_char *subsymtext, 
        const wg_char *subtext, wg_char **new_string);

WG_PUBLIC void
wg_fptr_2_str(fvoid ptr, wg_char *cstr);

#endif
