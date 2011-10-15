#ifndef _WG_STRING_H
#define _WG_STRING_H

WG_PUBLIC wg_size
wg_strlen(wg_char *string);

WG_PUBLIC wg_status
wg_strcpy(wg_char *dest, const wg_char *src);

WG_PUBLIC wg_status
wg_strdup(wg_char *string, wg_char **copied);

#endif
