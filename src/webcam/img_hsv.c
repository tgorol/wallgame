#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <math.h>

#include <gtk/gtk.h>

#include <wgtypes.h>
#include <wg.h>
#include <wgmacros.h>

#include <linux/videodev2.h>

#include "include/cam.h"
#include "include/img.h"
#include "include/img_hsv.h"
#include "include/img_bgrx.h"
#include "include/img_rgb24.h"


#define MAX3(r, g, b) WG_MAX(b , WG_MAX(r, g))
#define MIN3(r, g, b) WG_MIN(b , WG_MIN(r, g))

#define RAD_2_CART(val)  ((2.0 * M_PI * (val)) / 360.0)
#define CART_2_RAD(val)  (((val) *  360.0) / (2.0 * M_PI))

#define CART_2_RAD_COFF  WG_FLOAT(57.29577951)

WG_PRIVATE float atan2_fast(float y, float x);

/*! @defgroup image_hsv HSV manipulation
 * @ingroup image
 * @{ 
 */

/**
 * @brief Convert BGRX to HSV
 *
 * @param rgb_img  BGRX image
 * @param hsv_img   Memory to store HSV image
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
img_rgb_2_hsv(Wg_image *rgb_img, Wg_image *hsv_img)
{
    cam_status status = CAM_FAILURE;
    rgb24_pixel  *rgb_pixel = NULL;
    Hsv          *hsv_pixel = NULL;
    wg_int row = 0;
    wg_int col = 0;
    wg_uint width = 0;
    wg_uint height = 0;
    wg_float rgb_max = WG_FLOAT(0.0);
    wg_float rgb_min = WG_FLOAT(0.0);
    wg_float r = WG_FLOAT(0.0); 
    wg_float g = WG_FLOAT(0.0);
    wg_float b = WG_FLOAT(0.0);

    CHECK_FOR_NULL_PARAM(rgb_img);
    CHECK_FOR_NULL_PARAM(hsv_img);

    if (rgb_img->type != IMG_RGB){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                rgb_img->type, IMG_RGB);
        return CAM_FAILURE;
    }

    img_get_width(rgb_img, &width);
    img_get_height(rgb_img, &height);

    status = img_fill(width, height, sizeof (Hsv), IMG_HSV, hsv_img);
    if (CAM_SUCCESS != status){
        return status;
    }

    for (row = 0; row < height; ++row){
        img_get_row(rgb_img, row, (wg_uchar**)&rgb_pixel);
        img_get_row(hsv_img, row, (wg_uchar**)&hsv_pixel);
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
 * @brief Convert BGRX to HSV
 *
 * @param rgb_img  BGRX image
 * @param hsv_img   Memory to store HSV image
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
img_rgb_2_hsv_gtk(Wg_image *rgb_img, Wg_image *hsv_img)
{
    cam_status status = CAM_FAILURE;
    register rgb24_pixel  *rgb_pixel = NULL;
    register Hsv          *hsv_pixel = NULL;
    wg_uchar *tmp_ptr = NULL;
    gdouble r = 0.0;
    gdouble g = 0.0;
    gdouble b = 0.0;
    wg_int row = 0;
    wg_int col = 0;
    wg_uint width = 0;
    wg_uint height = 0;

    CHECK_FOR_NULL_PARAM(rgb_img);
    CHECK_FOR_NULL_PARAM(hsv_img);

    if (rgb_img->type != IMG_RGB){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                rgb_img->type, IMG_RGB);
        return CAM_FAILURE;
    }

    img_get_width(rgb_img, &width);
    img_get_height(rgb_img, &height);

    status = img_fill(width, height, sizeof (Hsv), IMG_HSV, hsv_img);
    if (CAM_SUCCESS != status){
        return status;
    }

    for (row = 0; row < height; ++row){
        img_get_row(rgb_img, row, &tmp_ptr);
        rgb_pixel = (rgb24_pixel*)tmp_ptr;
        img_get_row(hsv_img, row, &tmp_ptr);
        hsv_pixel = (Hsv*)tmp_ptr;

        /* Calculate HSV value for each pixel and update uint values */
        for (col = 0; col < width; ++col, ++hsv_pixel, ++rgb_pixel){

            r = PIXEL_RED(*rgb_pixel) / 255.0;
            g = PIXEL_GREEN(*rgb_pixel) / 255.0;
            b = PIXEL_BLUE(*rgb_pixel) / 255.0;

            gtk_rgb_to_hsv(r, g, b, 
                    &hsv_pixel->hue,
                    &hsv_pixel->sat,
                    &hsv_pixel->val
                    );

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
img_rgb_2_hsv_fast(Wg_image *rgb_img, Wg_image *hsv_img)
{
    cam_status status = CAM_FAILURE;
    register rgb24_pixel  *rgb_pixel = NULL;
    register Hsv          *hsv_pixel = NULL;
    wg_uchar *tmp_ptr = NULL;
    wg_int32 r = 0;
    wg_int32 g = 0;
    wg_int32 b = 0;
    wg_int row = 0;
    wg_int col = 0;
    wg_uint width = 0;
    wg_uint height = 0;
    wg_float alpha = WG_FLOAT(0.0);
    wg_float beta = WG_FLOAT(0.0);
    wg_int   v1 = 0;
    wg_int   v2 = 0;

    /* sqrt(3.0) = 1.732050808  */
    static const wg_float coff_b = WG_FLOAT(1.732050808 / (2.0 * WG_UCHAR_MAX));
    static const wg_float coff_a = WG_FLOAT(1.0 / (2.0  * WG_UCHAR_MAX));

    CHECK_FOR_NULL_PARAM(rgb_img);
    CHECK_FOR_NULL_PARAM(hsv_img);

    if (rgb_img->type != IMG_RGB){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                rgb_img->type, IMG_RGB);
        return CAM_FAILURE;
    }

    img_get_width(rgb_img, &width);
    img_get_height(rgb_img, &height);

    status = img_fill(width, height, sizeof (Hsv), IMG_HSV, hsv_img);
    if (CAM_SUCCESS != status){
        return status;
    }

    for (row = 0; row < height; ++row){
        img_get_row(rgb_img, row, &tmp_ptr);
        rgb_pixel = (rgb24_pixel*)tmp_ptr;
        img_get_row(hsv_img, row, &tmp_ptr);
        hsv_pixel = (Hsv*)tmp_ptr;

        /* Calculate HSV value for each pixel and update uint values */
        for (col = 0; col < width; ++col, ++hsv_pixel, ++rgb_pixel){

            r = PIXEL_RED(*rgb_pixel);
            g = PIXEL_GREEN(*rgb_pixel);
            b = PIXEL_BLUE(*rgb_pixel);

            v1 = MAX3(r, g, b);

            hsv_pixel->val = WG_FLOAT(v1);
            if (hsv_pixel->val == WG_FLOAT(0.0)){
                hsv_pixel->hue = hsv_pixel->sat = WG_FLOAT(0.0);
                continue;
            }

            /* for this values dividing by WG_UCHAR_MAX is already incorporated
             * in coff_b and coff_a
             */
            v1 = ((r << 1) - g - b);
            v2 = g - b;
            alpha = WG_FLOAT(coff_a * WG_FLOAT(v1));
            beta  = WG_FLOAT(coff_b * WG_FLOAT(v2));

            hsv_pixel->sat = WG_FLOAT(sqrt(alpha * alpha + beta * beta));
            if (hsv_pixel->sat == WG_FLOAT(0.0)){
                hsv_pixel->hue = WG_FLOAT(0.0);
                continue;
            }

            hsv_pixel->hue = atan2_fast(CART_2_RAD(beta), CART_2_RAD(alpha));
        }
    }

    return CAM_SUCCESS;
}

/**
 * @brief Convert BGRX to HSV
 *
 * @param bgrx_img  BGRX image
 * @param hsv_img   Memory to store HSV image
 *
 * @retval CAM_SUCCESS
 * @retval CAM_FAILURE
 */
cam_status
img_bgrx_2_hsv_fast(Wg_image *bgrx_img, Wg_image *hsv_img)
{
    register bgrx_pixel  *bgrx_pix = NULL;
    register Hsv         *hsv_pixel = NULL;
    wg_uchar *tmp_ptr = NULL;
    cam_status status = CAM_FAILURE;
    wg_int row = 0;
    wg_int col = 0;
    wg_uint width = 0;
    wg_uint height = 0;
    wg_float alpha = WG_FLOAT(0.0);
    wg_float beta = WG_FLOAT(0.0);
    wg_float r = WG_FLOAT(0.0);
    wg_float g = WG_FLOAT(0.0);
    wg_float b = WG_FLOAT(0.0);

    /* sqrt(3.0) = 1.732050808  */
    static const wg_float coff_b = WG_FLOAT(1.732050808 / (2.0 * WG_UCHAR_MAX));
    static const wg_float coff_a = WG_FLOAT(1.0 / (2.0  * WG_UCHAR_MAX));

    CHECK_FOR_NULL_PARAM(bgrx_img);
    CHECK_FOR_NULL_PARAM(hsv_img);

    if (bgrx_img->type != IMG_BGRX){
        WG_ERROR("Invalig image format! Passed %d expect %d\n", 
                bgrx_img->type, IMG_BGRX);
        return CAM_FAILURE;
    }

    img_get_width(bgrx_img, &width);
    img_get_height(bgrx_img, &height);

    status = img_fill(width, height, sizeof (Hsv), IMG_HSV, hsv_img);
    if (CAM_SUCCESS != status){
        return status;
    }

    for (row = 0; row < height; ++row){
        img_get_row(bgrx_img, row, &tmp_ptr);
        bgrx_pix = (bgrx_pixel*)tmp_ptr;
        img_get_row(hsv_img, row, &tmp_ptr);
        hsv_pixel = (Hsv*)tmp_ptr;

        for (col = 0; col < width; ++col, ++hsv_pixel, ++bgrx_pix){

            r = WG_FLOAT(BGRX_R(bgrx_pix));
            g = WG_FLOAT(BGRX_G(bgrx_pix));
            b = WG_FLOAT(BGRX_B(bgrx_pix));

            hsv_pixel->val = MAX3(r, g, b);
            if (hsv_pixel->val == WG_FLOAT(0.0)){
                hsv_pixel->hue = hsv_pixel->sat = WG_FLOAT(0.0);
                continue;
            }

            alpha = coff_a * ((r * WG_FLOAT(2.0)) - g - b);
            beta  = coff_b * (g - b);

            hsv_pixel->sat = sqrt(alpha * alpha + beta * beta);
            if (hsv_pixel->sat == WG_FLOAT(0.0)){
                hsv_pixel->hue = WG_FLOAT(0.0);
                continue;
            }

            hsv_pixel->hue = atan2_fast(CART_2_RAD_COFF * beta , CART_2_RAD_COFF * alpha);

        }
    }

    return CAM_SUCCESS;
}

WG_PRIVATE wg_float 
atan2_fast(wg_float y, wg_float x)
{
    wg_float r;
    wg_float angle;
    static const wg_float coeff_1 = M_PI / 4.0;
    static const wg_float coeff_2 = 3.0 * M_PI / 4.0;

    wg_float abs_y = fabs(y)+1e-10;      // kludge to prevent 0/0 condition
        if (x >= 0.0)
        {
            r = (x - abs_y) / (x + abs_y);
            angle = coeff_1 - coeff_1 * r;
//            angle = 0.1963 * r * r * r - 0.9817 * r + coeff_1;
        }
        else
        {
            r = (x + abs_y) / (abs_y - x);
            angle = coeff_2 - coeff_1 * r;
//            angle = 0.1963 * r * r * r - 0.9817 * r + coeff_2;
        }
    if (y < 0.0)
        return(-angle);     // negate if in quad III or IV
    else
        return(angle);
}

/*! @} */
