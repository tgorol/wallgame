#ifndef _WGTYPES_H
#define _WGTYPES_H

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

typedef void (*fvoid)(void)         ;

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
