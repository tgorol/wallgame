#ifndef _WG_STRING_H
#define _WG_STRING_H

WG_PUBLIC wg_size
wg_strlen(wg_char *string);

WG_PUBLIC wg_status
wg_strcpy(wg_char *dest, const wg_char *src);

WG_PUBLIC wg_status
wg_strdup(wg_char *string, wg_char **copied);

WG_PUBLIC wg_boolean
wg_strcmpchar(wg_char *string, wg_char c, wg_char **last);

WG_PUBLIC wg_boolean
wg_strncmp(wg_char *str1, wg_char *str2, wg_size len, wg_char **last);

WG_PUBLIC wg_status
wg_substitute(wg_char *string, wg_char subsym, wg_char *subsymtext, 
        wg_char *subtext, wg_char **new_string);

#endif
