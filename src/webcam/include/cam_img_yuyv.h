#ifndef _CAM_IMG_YUYV_H
#define _CAM_IMG_YUYV_H

WG_PUBLIC cam_status
cam_img_yuyv_2_rgb24(wg_uchar *in_buffer, wg_ssize in_size, 
        wg_uint width, wg_uint height, Wg_image *img);


#endif
