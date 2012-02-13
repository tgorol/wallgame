#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <setjmp.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <linux/videodev2.h>

#include "include/cam.h"
#include "include/img.h"
#include "include/img_yuyv.h"

/*! @defgroup webcam_yuyv YUYV Conversion Functions
 *  @ingroup image
 */

/*! @{ */

/** @brief interlan use */
#define C(Y)    ((Y) - 16)

/** @brief interlan use */
#define D(U)    ((U) - 128)

/** @brief interlan use */
#define E(V)    ((V) - 128)

/** @brief interlan use */
#define BLUE_E(E)         (             409 * (E) + 128)

/** @brief interlan use */
#define GREEN_DE(D, E)    (-100 * (D) - 208 * (E) + 128)

/** @brief interlan use */
#define RED_D(D)          ( 516 * (D)             + 128)

/** @brief interlan use */
#define BLUE(C, BE)                                             \
    clamp_0_255(((298 * (C) + (BE)) >> 8))

/** @brief interlan use */
#define GREEN(C, GDE)                                            \
    clamp_0_255(((298 * (C) + (GDE)) >> 8))

/** @brief interlan use */
#define RED(C, RD)                                              \
    clamp_0_255(((298 * (C) + (RD)) >> 8))

/** Pointer to the yuyv component */
typedef wg_uchar (*Component)[YUYV_COMPONENT_NUM];

WG_PRIVATE wg_uchar clamp_0_255(wg_int value);

/**
 * @brief Convert YUYV(YUV4:2:2) to RGB888
 *
 * @param in_buffer YUYV image buffer
 * @param in_size   size of the YUYV buffer
 * @param width     width of the picture in picels
 * @param height    height of the picture in pixels
 * @param img       image structure to store converted image
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
img_yuyv_2_rgb24(wg_uchar *in_buffer, wg_ssize in_size, 
        wg_uint width, wg_uint height, Wg_image *img)
{
    register wg_uchar *outbuf = NULL;
    register Component pixbuf = NULL;
    wg_uchar  *component;
    wg_uint width_count = 0;
    wg_int  BE, GDE, RD;
    wg_int  C0, C1, D, E;
    wg_int comp_num = 0;

    pixbuf = (Component)in_buffer;

    img_fill(width, height, 3, IMG_RGB, img);

    outbuf = img->image;

    comp_num = (width * height) >> 1;

    /* Each 2 pixels are made out of 4 bytes
     * Y0 V Y1 U   
     * Y0 and Y1 - luminations for 2 pixels
     * V and U   - belong to both pixels
     */
    for (width_count = 0; width_count < comp_num; ++width_count, ++pixbuf){
        component = *pixbuf;

        C0 = C(component[POS_Y0]);
        C1 = C(component[POS_Y1]);
        D  = D(component[POS_U]);
        E  = E(component[POS_V]);

        RD  = RED_D(D);
        GDE = GREEN_DE(D, E);
        BE  = BLUE_E(E);

        outbuf[0] = RED(C0, RD);
        outbuf[1] = GREEN(C0, GDE);
        outbuf[2] = BLUE(C0, BE);

        outbuf[3] = RED(C1, RD);
        outbuf[4] = GREEN(C1, GDE);
        outbuf[5] = BLUE(C1, BE);

        outbuf += 6;
    }

    return CAM_SUCCESS;
}

WG_PRIVATE wg_uchar
clamp_0_255(wg_int value)
{
    __asm__ __volatile__(
            "cmpl $255, %1\n\t"
            "jg 1f\n\t"
            "cmpl $0, %1\n\t"
            "jge 2f\n\t"
            "xorl %0, %0\n\t"
            "jmp 2f\n\t"
            "1:\tmovl $255, %0\n\t"
            "2:"
            : "=a"(value)
            : "a"(value)
           );

    return (wg_uchar)value;

#if 0
    if (value > 255){
        return 255;
    }
    else if (value < 0){
        return 0;
    }

    return value;
#endif
}

/*! @} */
