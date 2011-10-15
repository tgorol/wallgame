#include <string.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <wg_string.h>


wg_size
wg_strlen(wg_char *string)
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
