#ifndef _WG_STRING_H
#define _WG_STRING_H

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

#endif
