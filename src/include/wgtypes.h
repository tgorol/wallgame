#ifndef _WGTYPES_H
#define _WGTYPES_H

#include <math.h>
#include <limits.h>
#include <stdint.h>
/*! @todo Remove this include */

typedef char           wg_char      ;
typedef signed char    wg_schar     ;
typedef unsigned char  wg_uchar     ;

typedef short          wg_short     ;
typedef signed short   wg_sshort    ;
typedef unsigned short wg_ushort    ;

typedef signed int     wg_sint      ;
typedef unsigned int   wg_uint      ;
typedef int            wg_int       ;

typedef float          wg_float     ;
typedef double         wg_double    ;

typedef size_t         wg_size      ;
typedef ssize_t        wg_ssize     ;

typedef long int       wg_long      ;

typedef uint64_t       wg_uint64    ;
typedef int64_t        wg_int64     ;

typedef uint32_t       wg_uint32    ;
typedef int32_t        wg_int32     ;

typedef uint16_t       wg_uint16    ;
typedef int16_t        wg_int16     ;

typedef uint8_t        wg_uint8    ;
typedef int8_t         wg_int8     ;

#define WG_UCHAR_MAX   UCHAR_MAX
#define WG_CHAR_MAX    CHAR_MAX

typedef void (*fvoid)(void)         ;

#define WG_FLOAT(val)  ((wg_float)val)
#define WG_DOUBLE(val)  ((wg_double)val)

#define DOUBLE(val)  ((double)val)
#define FLOAT(val)  ((float)val)

#define WG_INT(val) ((wg_int)val)
#define WG_UINT(val) ((wg_uint)val)

typedef enum WG_BOOLEAN {
    WG_FALSE = (1 == 0), 
    WG_TRUE  = (1 == 1)
} wg_boolean;

typedef enum WG_STATUS {
    WG_SUCCESS  = 0  , 
    WG_FAILURE       ,
    WG_NO       = 0  ,
    WG_YES           ,
} wg_status;

#endif
