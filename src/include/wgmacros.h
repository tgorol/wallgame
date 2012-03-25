#ifndef _WGMACROS_H
#define _WGMACROS_H

#include <stdio.h>
#include <stdlib.h>
#include <wg_memleak.h>

/** @brief Print message on stdout */
#define WG_PRINT(...)                                     \
    fprintf(stdout, __VA_ARGS__)                      

/** @brief Print message on stderr */
#define WG_PRINT_ERR(...)                                 \
    fprintf(stderr, __VA_ARGS__)

/** @brief Print error message */
#define WG_ERROR(...)                                     \
    WG_PRINT_ERR("ERROR: %s:%d ", __PRETTY_FUNCTION__,    \
            __LINE__);                                    \
    WG_PRINT_ERR(__VA_ARGS__)

/** @brief Print warning log message */
#define WG_LOG(...)                                       \
    WG_PRINT("LOG: ");                                    \
    WG_PRINT(__VA_ARGS__)

/** @brief Print warning message */
#define WG_WARN(...)                                      \
    WG_PRINT("WARNINIG: ");                             \
    WG_PRINT(__VA_ARGS__)                      

/** @brief Print debug message     */
#ifndef WGDEBUG
#define WG_DEBUG(...)                
#else
#define WG_DEBUG(...)                \
    WG_PRINT("DEBUG: ");      \
    WG_PRINT(__VA_ARGS__)                      
#endif

/** @biref If value == NULL return WG_FAILURE
 */
#define CHECK_FOR_NULL(param)                    \
    if (NULL == (param)){                        \
        WG_ERROR("NULL parameter: " #param"\n"); \
        return WG_FAILURE;                       \
    }


/** @brief Check for null parameters.  
 */
#ifdef RELEASE
#define CHECK_FOR_NULL_PARAM(param) CHECK_FOR_NULL(param)
#else
#define CHECK_FOR_NULL_PARAM(param)                    
#endif

/** @todo addRANGE_LT_PARAM */
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
        WG_ERROR("Failed on: " #cond"\n");         \
        return WG_FAILURE;                         \
    }

#ifdef MEMLEAK_CHECK
#define MEMLEAK_START wg_memleak_start()
#else
#define MEMLEAK_START
#endif

#ifdef MEMLEAK_CHECK
#define MEMLEAK_STOP wg_memleak_stop()
#else
#define MEMLEAK_STOP
#endif

#ifdef MEMLEAK_CHECK
#define WG_MALLOC(size)  wg_malloc(size, __FILE__, __LINE__)
#else
#define WG_MALLOC(size)  malloc(size)
#endif

#ifdef MEMLEAK_CHECK
#define WG_MALLOC_PTR (&wg_malloc)
#else
#define WG_MALLOC_PTR (&malloc)
#endif

#ifdef MEMLEAK_CHECK
#define WG_CALLOC(num, size)  wg_calloc(num, size,  __FILE__, __LINE__)
#else
#define WG_CALLOC(num, size) calloc(num, size)
#endif

#ifdef MEMLEAK_CHECK
#define WG_CALLOC_PTR (&wg_calloc)
#else
#define WG_CALLOC_PTR (&calloc)
#endif

#ifdef MEMLEAK_CHECK
#define WG_FREE(ptr)                                       \
    do{                                                    \
        __typeof__ (ptr) p = ptr;                          \
        if (NULL != p){                                    \
            wg_free(p, WG_FALSE);                                       \
            p = NULL;                                      \
        }                                                  \
    }while (0)
#else
#define WG_FREE(ptr)                                       \
    do{                                                    \
        __typeof__ (ptr) p = ptr;                          \
        if (NULL != p){                                    \
            free(p);                                       \
            p = NULL;                                      \
        }                                                  \
    }while (0)
#endif

#define WG_ALLOCA(size)                                    \
    alloca(size)

#ifdef MEMLEAK_CHECK
#define WG_FREE_PTR (&wg_free)
#else
#define WG_FREE_PTR (&free)
#endif

#define WG_ZERO_STRUCT(p)                                  \
    memset(p, '\0', sizeof (*p))

#define STRING_EMPTY(string)  (*(string) == '\0')

#define STRING_NOT_EMPTY(string)  (*(string) != '\0')

#define WG_MAX(a, b)                                          \
    ((a) > (b) ? (a) : (b)) 
    
#define WG_MIN(a, b)                                          \
    ((a) < (b) ? (a) : (b)) 

#define STRI(s) #s
#define STR(s) STRI(s)

#define WG_SIGN(val) ((val) > 0 ? 1 : -1)

#define ELEMNUM(array)                                    \
    (sizeof (array) / sizeof (array[0]))


#define SIZE_1B     (1 << 0)
#define SIZE_2B     (1 << 1)
#define SIZE_4B     (1 << 2)
#define SIZE_8B     (1 << 3)
#define SIZE_16B    (1 << 4)
#define SIZE_32B    (1 << 5)
#define SIZE_64B    (1 << 6)
#define SIZE_128B   (1 << 7)
#define SIZE_256B   (1 << 8)
#define SIZE_512B   (1 << 9)
#define SIZE_1KB    (1 << 10)

WG_INLINE void
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
