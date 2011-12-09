#ifndef _WGMACROS_H
#define _WGMACROS_H

#include <stdio.h>
#include <stdlib.h>

#define WG_ERROR(...)                               \
    {fprintf(stderr, "error: %s:%d ", __PRETTY_FUNCTION__, \
            __LINE__);                              \
    fprintf(stderr, __VA_ARGS__); }

#define WG_PRINT(...)                \
    fprintf(stdout, __VA_ARGS__)                      

#define WG_LOG(...)                  \
    fprintf(stdout,  "log  : ");     \
    fprintf(stdout, __VA_ARGS__)                      

#define WG_WARN(...)                      \
    fprintf(stdout,  "WARNINIG  : ");     \
    fprintf(stdout, __VA_ARGS__)                      

#ifndef WGDEBUG
#define WG_DEBUG(...)                
#else
#define WG_DEBUG(...)                \
    fprintf(stdout,  "debug: ");      \
    fprintf(stdout, __VA_ARGS__)                      
#endif

#define CHECK_FOR_NULL(param)                    \
    if (NULL == (param)){                        \
        WG_ERROR("NULL parameter: " #param"\n"); \
        return WG_FAILURE;                       \
    }

#ifdef RELEASE
#define CHECK_FOR_NULL_PARAM(param) CHECK_FOR_NULL(param)
#else
#define CHECK_FOR_NULL_PARAM(param)                    
#endif


#ifdef RELEASE
#define CHECK_FOR_RANGE_LT(param, val)                    
#else
#define CHECK_FOR_RANGE_LT(param, val)                     \
    if ((param) < (val)){                                  \
        WG_ERROR("Range error: " #param" lt " #val"\n");   \
        return WG_FAILURE;                                 \
    }
#endif

#ifdef RELEASE
#define CHECK_FOR_RANGE_GT(param, val)                     
#else
#define CHECK_FOR_RANGE_GT(param, val)                     \
    if ((param) > (val)){                                  \
        WG_ERROR("Range error: " #param" gt " #val"\n");   \
        return WG_FAILURE;                                 \
    }
#endif

#ifdef RELEASE
#define CHECK_FOR_RANGE_GE(param, val)                     
#else
#define CHECK_FOR_RANGE_GE(param, val)                     \
    if ((param) >= (val)){                                 \
        WG_ERROR("Range error: " #param" ge " #val"\n");   \
        return WG_FAILURE;                                 \
    }
#endif

#ifdef RELEASE
#define CHECK_FOR_RANGE_LE(param, val)                     
#else
#define CHECK_FOR_RANGE_LE(param, val)                     \
    if ((param) <= (val)){                                 \
        WG_ERROR("Range error: " #param" le " #val"\n");   \
        return WG_FAILURE;                                 \
    }
#endif

#define CHECK_FOR_FAILURE(param)                   \
    if (WG_FAILURE == (param)){                    \
        WG_ERROR("Status failure: " #param"\n");   \
        return WG_FAILURE;                         \
    }

#define CHECK_FOR_COND(cond)                       \
    if (!(cond)){                                  \
        WG_ERROR("Failed on: " #cond"\n");    \
        return WG_FAILURE;                         \
    }

#define WG_MALLOC(size)                                    \
    malloc(size)

#define WG_MALLOC_PTR (&malloc)

#define WG_CALLOC(num, size)                               \
    calloc(num, size)

#define WG_CALLOC_PTR (&calloc)

#define WG_FREE(ptr)                                       \
    do{                                                    \
        __typeof__ (ptr) p = ptr;                          \
        if (NULL != p){                                    \
            free(p);                                       \
            p = NULL;                                      \
        }                                                  \
    }while (0)

#define WG_ALLOCA(size)                                    \
    alloca(size)

#define WG_FREE_PTR (&free)

#define STRING_EMPTY(string)  (*(string) == '\0')

#define STRING_NOT_EMPTY(string)  (*(string) != '\0')

#define WG_MAX(a, b)                                          \
    ((a) > (b) ? (a) : (b)) 
    
#define WG_MIN(a, b)                                          \
    ((a) < (b) ? (a) : (b)) 

#define STRI(s) #s
#define STR(s) STRI(s)


#define ELEMNUM(array)                                    \
    (sizeof (array) / sizeof (array[0]))

inline WG_STATIC void
char_2_hex(wg_char c, wg_char *ret)
{
    static char hex_table[] = {
        '0', '1', '2', '3', '4', '5' ,'6', '7', 
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };

    ret[0] = hex_table[(c & 0xf0) >> 4];
    ret[1] = hex_table[c & 0x0f];

    return;
}

#endif
