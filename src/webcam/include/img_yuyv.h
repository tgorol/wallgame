#ifndef _CAM_IMG_YUYV_H
#define _CAM_IMG_YUYV_H

/** 
* @brief 
*/
enum {
    POS_Y0 = 0,  /*!< YO position */ 
    POS_V     ,  /*!< V  position */
    POS_Y1    ,  /*!< Y1 position */
    POS_U     ,  /*!< U  position */
    YUYV_COMPONENT_NUM  /*!< number of components per pixel in YUYV image */
};

WG_PUBLIC cam_status
img_yuyv_2_rgb24(wg_uchar *in_buffer, wg_ssize in_size, 
        wg_uint width, wg_uint height, Wg_image *img);


#endif
