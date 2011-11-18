#ifndef _WG_CAM_IMG_JPEG_H
#define _WG_CAM_IMG_JPEG_H


WG_PUBLIC wg_cam_status
wg_cam_img_jpeg_decompress(wg_uchar *in_buffer, wg_ssize in_size,
        wg_uchar **out_buffer, wg_ssize *out_size, wg_uint *width, 
        wg_uint *height);



#endif
