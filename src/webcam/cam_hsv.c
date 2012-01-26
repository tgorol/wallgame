#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <linux/videodev2.h>

#include "include/cam.h"
#include "include/cam_img.h"
#include "include/cam_hsv.h"


#define MAX3(r, g, b) WG_MAX(b , WG_MAX(r, g))
#define MIN3(r, g, b) WG_MIN(b , WG_MIN(r, g))

#define RAD_2_CART(val)  ((2.0 * M_PI * (val)) / 360.0)
#define CART_2_RAD(val)  (((val) *  360.0) / (2.0 * M_PI))

typedef struct Hsv{
    wg_float hue;
    wg_float sat;
    wg_float val;
}Hsv;

cam_status
cam_img_rgb_2_hsv(Wg_image *rgb_img, Wg_image *hsv_img)
{
    cam_status status = CAM_FAILURE;
    wg_int row;
    wg_int col;
    wg_uint width;
    wg_uint height;
    rgb24_pixel  *rgb_pixel;
    Hsv          *hsv_pixel;
    wg_float rgb_max;
    wg_float rgb_min;
    wg_float r; 
    wg_float g;
    wg_float b;

    CHECK_FOR_NULL_PARAM(rgb_img);
    CHECK_FOR_NULL_PARAM(hsv_img);

    cam_img_get_width(rgb_img, &width);
    cam_img_get_height(rgb_img, &height);

    status = cam_img_fill(width, height, sizeof (Hsv), hsv_img);
    if (CAM_SUCCESS != status){
        return status;
    }

    for (row = 0; row < height; ++row){
        cam_img_get_row(rgb_img, row, (wg_uchar**)&rgb_pixel);
        cam_img_get_row(hsv_img, row, (wg_uchar**)&hsv_pixel);
        for (col = 0; col < width; ++col, ++hsv_pixel){
            rgb_max = MAX3(
                    PIXEL_RED(rgb_pixel[col]),
                    PIXEL_GREEN(rgb_pixel[col]),
                    PIXEL_BLUE(rgb_pixel[col])
                    );
            rgb_min = MIN3(
                    PIXEL_RED(rgb_pixel[col]),
                    PIXEL_GREEN(rgb_pixel[col]),
                    PIXEL_BLUE(rgb_pixel[col])
                    );

            hsv_pixel->val = rgb_max;
            if (rgb_max == WG_FLOAT(0.0)){
                hsv_pixel->hue = hsv_pixel->sat = WG_FLOAT(0.0);
                continue;
            }

            r = PIXEL_RED(rgb_pixel[col]);
            g = PIXEL_GREEN(rgb_pixel[col]);
            b = PIXEL_BLUE(rgb_pixel[col]);

            /* normnalize colors to 1 */
            r /= hsv_pixel->val;
            g /= hsv_pixel->val;
            b /= hsv_pixel->val;

            rgb_min /= hsv_pixel->val;
            rgb_max = WG_FLOAT(1.0);

            hsv_pixel->sat = rgb_max - rgb_min;
            if (hsv_pixel->sat == WG_FLOAT(0.0)){
                hsv_pixel->hue = WG_FLOAT(0.0);
                continue;
            }

            r = (r - rgb_min) / hsv_pixel->sat;
            g = (g - rgb_min) / hsv_pixel->sat;
            b = (b - rgb_min) / hsv_pixel->sat;
            rgb_min = MIN3(r, g, b);
            rgb_max = MAX3(r, g, b);

            if (rgb_max == r){
                hsv_pixel->hue = 0.0 + 60.0 * (g - b);
                if (hsv_pixel->hue < WG_FLOAT(0.0)){
                    hsv_pixel->hue += 360.0;
                }
            }else if (rgb_max == g){
                hsv_pixel->hue = 120.0 + 60 * (b - r);
            }else {
                hsv_pixel->hue = 240.0 + 60 * (r - g);
            }
        }
    }

    return CAM_SUCCESS;
}

/**
 * @brief Convert RGB24 to HSV
 *
 * @param rgb_img    RGB24 image
 * @param hsv_img    memory to store HSV image
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
cam_img_rgb_2_hsv_fast(Wg_image *rgb_img, Wg_image *hsv_img)
{
    register rgb24_pixel  *rgb_pixel;
    register Hsv          *hsv_pixel;
    wg_uchar *tmp_ptr;
    cam_status status = CAM_FAILURE;
    wg_int row;
    wg_int col;
    wg_uint width;
    wg_uint height;
    wg_float alpha;
    wg_float beta;
    wg_float r, g, b;

    /* sqrt(3.0) = 1.732050808  */
    static const wg_float coff_b = WG_FLOAT(1.732050808 / (2.0 * WG_UCHAR_MAX));
    static const wg_float coff_a = WG_FLOAT(1.0 / (2.0  * WG_UCHAR_MAX));

    CHECK_FOR_NULL_PARAM(rgb_img);
    CHECK_FOR_NULL_PARAM(hsv_img);

    cam_img_get_width(rgb_img, &width);
    cam_img_get_height(rgb_img, &height);

    status = cam_img_fill(width, height, sizeof (Hsv), hsv_img);
    if (CAM_SUCCESS != status){
        return status;
    }

    for (row = 0; row < height; ++row){
        cam_img_get_row(rgb_img, row, &tmp_ptr);
        rgb_pixel = (rgb24_pixel*)tmp_ptr;
        cam_img_get_row(hsv_img, row, &tmp_ptr);
        hsv_pixel = (Hsv*)tmp_ptr;

        for (col = 0; col < width; ++col, ++hsv_pixel){
            r = WG_FLOAT(PIXEL_RED(rgb_pixel[col]));
            g = WG_FLOAT(PIXEL_GREEN(rgb_pixel[col]));
            b = WG_FLOAT(PIXEL_BLUE(rgb_pixel[col]));

            hsv_pixel->val = MAX3(r, g, b);
            if (hsv_pixel->val == WG_FLOAT(0.0)){
                hsv_pixel->hue = hsv_pixel->sat = WG_FLOAT(0.0);
                continue;
            }

            /* for this values dividing by WG_UCHAR_MAX is already incorporated
             * in coff_b and coff_a
             */
            alpha = coff_a * ((r * WG_FLOAT(2.0)) - g - b);
            beta  = coff_b * (g - b);

            hsv_pixel->sat = sqrt(alpha * alpha + beta * beta);
            if (hsv_pixel->sat == WG_FLOAT(0.0)){
                hsv_pixel->hue = WG_FLOAT(0.0);
                continue;
            }

            hsv_pixel->hue = atan2(CART_2_RAD(beta), CART_2_RAD(alpha));
        }
    }

    return CAM_SUCCESS;
}


