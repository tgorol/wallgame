#ifndef _CAM_IMG_JPEG_H
#define _CAM_IMG_JPEG_H


WG_PUBLIC cam_status
cam_img_jpeg_decompress(wg_uchar *in_buffer, wg_ssize in_size,
        wg_uint width, wg_uint height, Wg_image *img);

WG_PUBLIC cam_status
cam_img_cleanup(Wg_image *img);


#endif
