#ifndef _CAM_FORMAT_SELECTOR_H
#define _CAM_FORMAT_SELECTOR_H

WG_PUBLIC wg_status
cam_select_decompressor(Wg_camera *cam);

WG_PUBLIC wg_status
cam_select_user_decompressor(Wg_camera *cam, __u32 pixelformat);

WG_PUBLIC cam_status
cam_get_decompressor(__u32 pixelformat, Wg_cam_decompressor *decomp);

WG_PUBLIC wg_boolean
cam_is_format_supported(__u32 pixelformat);

#endif
