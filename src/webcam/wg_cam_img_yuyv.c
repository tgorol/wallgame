#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <setjmp.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <linux/videodev2.h>

#include "include/cam.h"
#include "include/cam_img_yuyv.h"

#define C(Y)    ((Y) - 16)

#define D(U)    ((U) - 128)

#define E(V)    ((V) - 128)

#define BLUE(Y, V, U, C, D, E)                                             \
    clamp(((298 * (C) + 409 * (E) + 128) >> 8))

#define GREEN(Y, V, U, C, D, E)                                            \
    clamp(((298 * (C) - 100 * (D) - 208 * (E) + 128) >> 8))

#define RED(Y, V, U, C, D, E)                                              \
    clamp(((298 * (C) + 516 * (D) + 128) >> 8))

enum {
    POS_Y0 = 0,
    POS_V     ,
    POS_Y1    ,
    POS_U     ,
    COMPONENT_NUM
};

typedef wg_uchar (*Component)[COMPONENT_NUM];

WG_PRIVATE cam_status
allocate_output_buffer(wg_int width, wg_int height, wg_int comp_num, 
        wg_uchar **buffer);

WG_PRIVATE wg_uchar
clamp(wg_int value);

cam_status
cam_img_yuyv_2_rgb24(wg_uchar *in_buffer, wg_ssize in_size, 
        wg_uint width, wg_uint height, Wg_image *img)
{
    Component pixbuf = NULL;
    wg_uchar  *component;
    wg_uint width_count = 0;
    wg_int rgb_count = 0;
    wg_char R = 0;
    wg_char G = 0;
    wg_char B = 0;
    wg_int  C, D, E, V, U, Y;
    wg_uchar *outbuf = NULL;
    pixbuf = (Component)in_buffer;
    wg_int comp_num = 0;

    allocate_output_buffer(width, height, 3, &outbuf);

    comp_num = (width * height) >> 1;

    /* Each 2 pixels are made out of 4 bytes
     * Y0 V Y1 U   
     * Y0 and Y1 - luminations for 2 pixels
     * V and U   - belong to both pixels
     */
    for (width_count = 0; width_count < comp_num; ++width_count){
        component = pixbuf[width_count];

        Y = component[POS_Y0];
        V = component[POS_V];
        U = component[POS_U];
        C = C(Y);
        D = D(U);
        E = E(V);

        R = RED(Y, V, U, C, D, E);
        G = GREEN(Y, V, U, C, D, E);
        B = BLUE(Y, V, U, C, D, E);
       
        outbuf[rgb_count++] = R;
        outbuf[rgb_count++] = G;
        outbuf[rgb_count++] = B;

        Y = component[POS_Y1];
        C = C(Y);

        R = RED(Y, V, U, C, D, E);
        G = GREEN(Y, V, U, C, D, E);
        B = BLUE(Y, V, U, C, D, E);

        outbuf[rgb_count++] = R;
        outbuf[rgb_count++] = G;
        outbuf[rgb_count++] = B;
    }

    img->image = outbuf;
    img->width                = width;
    img->height               = height;
    img->size                 = height * width * 3;
    img->components_per_pixel = 3;
    img->rows                 = NULL;
    img->row_distance         = width * 3;

    return CAM_SUCCESS;
}

WG_PRIVATE cam_status
allocate_output_buffer(wg_int width, wg_int height, wg_int comp_num, 
        wg_uchar **buffer)
{
    wg_uchar *buf = NULL;

    CHECK_FOR_NULL_PARAM(buffer);

    buf = WG_CALLOC(width * height, comp_num);
    if (NULL == buf){
        return CAM_FAILURE;
    }

    *buffer = buf;

    return CAM_SUCCESS;
}

WG_PRIVATE wg_uchar
clamp(wg_int value)
{
    if (value > 255){
        return 255;
    }
    else if (value < 0){
        return 0;
    }

    return value;
}

